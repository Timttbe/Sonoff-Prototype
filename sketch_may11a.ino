#include <ESP8266WiFi.h>
#include <stdio.h>
#include "digest.h"
#include "client.h"

const char* ssid = "HanKoRoteador 2.0";
const char* password = "Tr@pical8888";

const char* host = "allcampobelo.ddns.net";
const int port = 10232;

const char* usuario = "admin";
const char* senha = "abc12345";

#define RELE_PIN 12   // D6 = GPIO12

WiFiClient client;

const char* uri = "/cgi-bin/eventManager.cgi?action=attach&codes=[VideoMotion]";

void setup() {
  Serial.begin(115200);

  pinMode(RELE_PIN, OUTPUT);
  digitalWrite(RELE_PIN, HIGH); // relê desligado, geralmente active LOW

  WiFi.begin(ssid, password);

  Serial.print("Conectando WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");

  conectarDVR();
}

void loop() {
  while (client.available()) {
    String linha = client.readStringUntil('\n');

    Serial.println(linha);

    if (linha.indexOf("Code=VideoMotion") >= 0 &&
        linha.indexOf("action=Start") >= 0) {

      Serial.println("MOVIMENTO!");

      digitalWrite(RELE_PIN, LOW);   // liga relê
      delay(5000);
      digitalWrite(RELE_PIN, HIGH);  // desliga relê
    }
  }

  if (!client.connected()) {
    Serial.println("Reconectando...");
    conectarDVR();
  }
}

void conectarDVR() {
  Serial.println("Obtendo challenge Digest...");

  WiFiClient authClient;

  if (!authClient.connect(host, port)) {
    Serial.println("Falha na conexão inicial");
    return;
  }

  authClient.print("GET ");
  authClient.print(uri);
  authClient.println(" HTTP/1.1");

  authClient.print("Host: ");
  authClient.println(host);

  authClient.println("Connection: close");
  authClient.println();

  String authLine = "";

  while (authClient.connected()) {
    String line = authClient.readStringUntil('\n');

    Serial.println(line);

    if (line.indexOf("WWW-Authenticate: Digest") >= 0) {
      authLine = line;
      authLine.replace("WWW-Authenticate: ", "");
      authLine.trim();
      break;
    }
  }

  authClient.stop();

  if (authLine == "") {
    Serial.println("Digest não encontrado");
    return;
  }

  Serial.println("Gerando Digest...");

  digest_t d;
  digest_init(&d);

  digest_client_parse(&d, authLine.c_str());

  digest_attr_value_t usernameValue;
  usernameValue.const_str = usuario;
  digest_set_attr(&d, D_ATTR_USERNAME, usernameValue);

  digest_attr_value_t passwordValue;
  passwordValue.const_str = senha;
  digest_set_attr(&d, D_ATTR_PASSWORD, passwordValue);

  digest_attr_value_t uriValue;
  uriValue.const_str = uri;
  digest_set_attr(&d, D_ATTR_URI, uriValue);

  digest_attr_value_t methodValue;
  methodValue.number = DIGEST_METHOD_GET;
  digest_set_attr(&d, D_ATTR_METHOD, methodValue);

  char result[1024];
  digest_client_generate_header(&d, result, sizeof(result));

  Serial.println("Header Digest:");
  Serial.println(result);

  Serial.println("Conectando autenticado...");

  if (!client.connect(host, port)) {
    Serial.println("Falha na conexão autenticada");
    return;
  }

  client.print("GET ");
  client.print(uri);
  client.println(" HTTP/1.1");

  client.print("Host: ");
  client.println(host);

  client.print("Authorization: ");
  client.println(result);

  client.println("Connection: keep-alive");
  client.println();

  Serial.println("Stream autenticado iniciado");
}