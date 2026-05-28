#include "esp8266.h"

static char rx_buf[ESP_RX_BUF_SIZE];
static volatile uint16_t rx_len = 0;
static volatile uint8_t  rx_done = 0;

/* UART接收中断回调 */
void ESP8266_UART_RxCallback(void) {
    uint8_t ch;
    if(__HAL_UART_GET_FLAG(&ESP_UART, UART_FLAG_RXNE)) {
        ch = (uint8_t)ESP_UART.Instance->DR;
        if(rx_len < ESP_RX_BUF_SIZE - 1) {
            rx_buf[rx_len++] = ch;
            rx_buf[rx_len] = 0;
        }
        if(ch == '\n') rx_done = 1;
    }
}

static void ClearBuf(void) {
    memset(rx_buf, 0, ESP_RX_BUF_SIZE);
    rx_len = 0; rx_done = 0;
}

const char* ESP8266_GetResponse(void) {
    return rx_buf;
}

/* 等待缓冲区中出现指定字符串 */
static uint8_t WaitFor(const char *expect, uint32_t timeout) {
    uint32_t tick = HAL_GetTick();
    while(HAL_GetTick() - tick < timeout) {
        if(rx_len > 0 && strstr(rx_buf, expect)) return 0;
    }
    return 1;
}

/* 发送AT命令，等待期望响应 */
static uint8_t SendAT(const char *cmd, const char *expect, uint32_t timeout) {
    ClearBuf();
    HAL_UART_Transmit(&ESP_UART, (uint8_t*)cmd, strlen(cmd), 1000);
    HAL_UART_Transmit(&ESP_UART, (uint8_t*)"\r\n", 2, 100);
    uint32_t tick = HAL_GetTick();
    while(HAL_GetTick() - tick < timeout) {
        if(rx_done) {
            rx_done = 0;
            if(strstr(rx_buf, expect)) return 0;
            if(strstr(rx_buf, "ERROR")) return 1;
        }
        if(rx_len > 0 && strstr(rx_buf, expect)) return 0;
    }
    return 2;
}

/* 通过AT+CIPSEND发送原始TCP数据 */
static uint8_t SendTCPData(uint8_t *data, uint16_t len) {
    char cmd[20];
    sprintf(cmd, "AT+CIPSEND=%d", len);
    ClearBuf();
    HAL_UART_Transmit(&ESP_UART, (uint8_t*)cmd, strlen(cmd), 1000);
    HAL_UART_Transmit(&ESP_UART, (uint8_t*)"\r\n", 2, 100);

    if(WaitFor(">", 3000)) return 1;

    ClearBuf();
    HAL_UART_Transmit(&ESP_UART, data, len, 3000);

    if(WaitFor("SEND OK", 5000)) return 2;
    return 0;
}

/* 写入MQTT字符串（2字节长度前缀 + 内容） */
static uint16_t WriteString(uint8_t *buf, const char *str) {
    uint16_t len = strlen(str);
    buf[0] = (len >> 8) & 0xFF;
    buf[1] = len & 0xFF;
    memcpy(&buf[2], str, len);
    return 2 + len;
}

/* 编码MQTT剩余长度字段 */
static uint8_t EncodeRemainLen(uint8_t *buf, uint16_t length) {
    uint8_t pos = 0;
    do {
        uint8_t byte = length % 128;
        length /= 128;
        if(length > 0) byte |= 0x80;
        buf[pos++] = byte;
    } while(length > 0);
    return pos;
}

/* 构建并发送MQTT CONNECT包 */
static uint8_t MQTTConnect(void) {
    uint8_t pkt[300];
    uint16_t pos;

    const char *clientId = ONENET_DEVICE_ID;
    const char *username = ONENET_PRODUCT_ID;
    const char *password = ONENET_TOKEN;

    // 计算剩余长度: 可变头(10) + clientId(2+len) + username(2+len) + password(2+len)
    uint16_t remainLen = 10
        + 2 + strlen(clientId)
        + 2 + strlen(username)
        + 2 + strlen(password);

    // 固定头
    pos = 0;
    pkt[pos++] = 0x10; // CONNECT类型
    pos += EncodeRemainLen(&pkt[pos], remainLen);

    // 可变头 - 协议名 "MQTT"
    pkt[pos++] = 0x00; pkt[pos++] = 0x04;
    pkt[pos++] = 'M';  pkt[pos++] = 'Q';
    pkt[pos++] = 'T';  pkt[pos++] = 'T';
    pkt[pos++] = 0x04; // 协议级别 MQTT 3.1.1
    pkt[pos++] = 0xC2; // 连接标志: Username+Password+CleanSession
    pkt[pos++] = 0x00; pkt[pos++] = 0x78; // Keep Alive 120秒

    // 载荷
    pos += WriteString(&pkt[pos], clientId);
    pos += WriteString(&pkt[pos], username);
    pos += WriteString(&pkt[pos], password);

    // 发送
    if(SendTCPData(pkt, pos)) return 1;

    // 等待CONNACK（不清空缓冲区，+IPD可能已随SEND OK一起到达）
    if(WaitFor("+IPD", 8000)) return 2;

    // 检查CONNACK: 在+IPD数据中查找 0x20(CONNACK类型)
    char *ipd = strstr(rx_buf, "+IPD");
    if(ipd) {
        char *colon = strchr(ipd, ':');
        if(colon) {
            uint8_t *data = (uint8_t*)(colon + 1);
            if(data[0] == 0x20 && data[3] != 0x00) {
                return 3; // CONNACK返回错误码
            }
        }
    }
    return 0;
}

/* 构建并发送MQTT SUBSCRIBE包 */
static uint8_t MQTTSubscribe(const char *topic) {
    uint8_t pkt[100];
    uint16_t pos;
    uint16_t topicLen = strlen(topic);
    uint16_t remainLen = 2 + 2 + topicLen + 1; // packetId + topic + qos

    pos = 0;
    pkt[pos++] = 0x82; // SUBSCRIBE类型
    pos += EncodeRemainLen(&pkt[pos], remainLen);
    pkt[pos++] = 0x00; pkt[pos++] = 0x01; // Packet ID = 1
    pos += WriteString(&pkt[pos], topic);
    pkt[pos++] = 0x01; // QoS 1

    if(SendTCPData(pkt, pos)) return 1;

    // 等待SUBACK（不清空缓冲区）
    if(WaitFor("+IPD", 5000)) return 2;
    return 0;
}

/* ========== 公共接口 ========== */

uint8_t ESP8266_Init(void) {
    HAL_Delay(2000);
    SendAT("ATE0", "OK", 2000); // 关闭回显
    if(SendAT("AT", "OK", 3000))          return 1;
    if(SendAT("AT+CWMODE=1", "OK", 2000)) return 2;
    char cmd[80];
    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"", WIFI_SSID, WIFI_PASS);
    if(SendAT(cmd, "WIFI GOT IP", 15000)) return 3;
    return 0;
}

uint8_t ESP8266_MQTTConnect(void) {
    // 关闭之前可能存在的TCP连接
    SendAT("AT+CIPCLOSE", "OK", 2000);
    HAL_Delay(500);

    // 建立TCP连接到MQTT Broker
    char cmd[80];
    sprintf(cmd, "AT+CIPSTART=\"TCP\",\"%s\",%s", ONENET_HOST, ONENET_PORT);
    if(SendAT(cmd, "CONNECT", 10000)) return 1;
    HAL_Delay(500);

    // 发送MQTT CONNECT包
    uint8_t ret = MQTTConnect();
    if(ret) return 10 + ret; // 11=发送失败, 12=无CONNACK, 13=认证失败

    HAL_Delay(300);

    // 订阅下行Topic
    ret = MQTTSubscribe(TOPIC_SUB);
    if(ret) return 20 + ret; // 21=发送失败, 22=无SUBACK

    return 0;
}

uint8_t ESP8266_PublishData(uint8_t temp, uint8_t humi) {
    char payload[128];
    uint8_t pkt[250];
    uint16_t pos;

    sprintf(payload,
        "{\"id\":\"1\",\"params\":{\"temperature\":{\"value\":%d},\"humidity\":{\"value\":%d}}}",
        temp, humi);

    uint16_t topicLen = strlen(TOPIC_PUB);
    uint16_t payloadLen = strlen(payload);
    uint16_t remainLen = 2 + topicLen + payloadLen;

    pos = 0;
    pkt[pos++] = 0x30; // PUBLISH QoS0
    pos += EncodeRemainLen(&pkt[pos], remainLen);
    pos += WriteString(&pkt[pos], TOPIC_PUB);
    memcpy(&pkt[pos], payload, payloadLen);
    pos += payloadLen;

    return SendTCPData(pkt, pos);
}

/* 二进制安全搜索（MQTT头含0x00字节，strstr会停住） */
static char* FindInBuf(const char *needle) {
    uint16_t nlen = strlen(needle);
    if(nlen == 0 || rx_len < nlen) return NULL;
    for(uint16_t i = 0; i + nlen <= rx_len; i++) {
        if(memcmp(&rx_buf[i], needle, nlen) == 0)
            return &rx_buf[i];
    }
    return NULL;
}

/* 发送set_reply响应给OneNet */
static uint8_t PublishSetReply(const char *id) {
    char payload[80];
    uint8_t pkt[200];
    uint16_t pos = 0;

    sprintf(payload, "{\"id\":\"%s\",\"code\":200,\"msg\":\"success\"}", id);

    uint16_t topicLen = strlen(TOPIC_SET_REPLY);
    uint16_t payloadLen = strlen(payload);
    uint16_t remainLen = 2 + topicLen + payloadLen;

    pkt[pos++] = 0x30;
    pos += EncodeRemainLen(&pkt[pos], remainLen);
    pos += WriteString(&pkt[pos], TOPIC_SET_REPLY);
    memcpy(&pkt[pos], payload, payloadLen);
    pos += payloadLen;

    return SendTCPData(pkt, pos);
}

uint8_t ESP8266_CheckLEDCmd(uint8_t *ledState) {
    /* 用二进制安全搜索查找"led" */
    char *ledP = FindInBuf("\"led\"");
    if(!ledP) return 0;

    /*
     * OneNET物模型布尔属性下发格式: {"id":"xxx","params":{"led":true/false}}
     * 找到 "led" 后，跳过冒号和空格，判断 true/false
     */
    char *colonP = strchr(ledP + 5, ':');
    if(!colonP) return 0;
    colonP++;
    while(*colonP == ' ') colonP++;

    if(*colonP == 't')       *ledState = 1;  /* true  -> LED ON  */
    else if(*colonP == 'f')  *ledState = 0;  /* false -> LED OFF */
    else if(*colonP == '1')  *ledState = 1;  /* 兼容数字格式 */
    else if(*colonP == '0')  *ledState = 0;
    else return 0; /* 无法识别的值 */

    /* 提取请求id */
    char reqId[20] = "1";
    char *idP = FindInBuf("\"id\":");
    if(idP) {
        idP += 4;
        while(*idP == ' ' || *idP == '"') idP++;
        char *idEnd = strchr(idP, '"');
        if(idEnd && (idEnd - idP) < 19) {
            memcpy(reqId, idP, idEnd - idP);
            reqId[idEnd - idP] = '\0';
        }
    }

    /* 提取PUBACK的packetId（QoS1） */
    uint16_t packetId = 0;
    char *ipd = FindInBuf("+IPD,");
    if(ipd) {
        char *colon = NULL;
        for(char *c = ipd; c < rx_buf + rx_len; c++) {
            if(*c == ':') { colon = c; break; }
        }
        if(colon) {
            uint8_t *m = (uint8_t*)(colon + 1);
            if((m[0] >> 4) == 3 && ((m[0] >> 1) & 3) >= 1) {
                uint16_t p = 1;
                while(m[p] & 0x80) p++;
                p++;
                uint16_t tlen = (m[p] << 8) | m[p+1];
                p += 2 + tlen;
                packetId = (m[p] << 8) | m[p+1];
            }
        }
    }

    /* 数据已提取，清空后发送响应 */
    ClearBuf();

    if(packetId > 0) {
        uint8_t puback[4] = {0x40, 0x02, (packetId>>8)&0xFF, packetId&0xFF};
        SendTCPData(puback, 4);
        HAL_Delay(100);
    }

    PublishSetReply(reqId);
    return 1;
}

uint8_t ESP8266_QueryVersion(void) {
    return SendAT("AT+GMR", "OK", 3000);
}