//___________________________________________________Bibliotecas______________________________________________________________

#include <SoftwareSerial.h>           //biblioteca das seriais RX/TX.
#include <TinyGPS.h>

//_________________________________________Configuração da porta serial______________________________________________________

SoftwareSerial serialGSM(10, 11);     // RX, TX ** serial para comunicação do modulo GSM.
SoftwareSerial serialGPS(6, 7);

//___________________________________________________Fynções______________________________________________________________

void configuraGSM();                  //Função para configurar o modulo SIM 800L.
void leGPS();                          //Função para coletar o posicionamento geografico.
void leGSM();                          //Função para piscar os leds e tocar a cirene.
void enviaSMS(String telefone, String mensagem);    //Função para enviar SMS.

//___________________________________________________Variaveis______________________________________________________________

TinyGPS gps;
String comandoGSM = "";
String ultimoGSM = "";
bool temSMS = false;
String telefoneSMS;
String dataHoraSMS;
String mensagemSMS;
#define senhaGsm "1234" 

//___________________________________________________SETUP______________________________________________________________

void setup() {
  
  Serial.begin(9600);                 //Baud "velocidade da serial monitor".
  serialGPS.begin(9600);              //Baud "velocidade da serial GPS".
  serialGSM.begin(9600);              //Baud "velocidade da serial GSM".

  Serial.println("Sketch Iniciado!");
  configuraGSM();
  leGPS();
}
//___________________________________________________LOOP______________________________________________________________

void loop() {
  // put your main code here, to run repeatedly:
  static unsigned long delayLeGPS = millis();

  leGSM();
  delay (100);
  
  if (comandoGSM != "") {
      //Serial.println("comandoGSM: ");
      //Serial.println(comandoGSM);
      ultimoGSM = comandoGSM;
      comandoGSM = "";
  }

  if (temSMS) {

    Serial.println("Chegou Mensagem!!");
    Serial.println();

    Serial.print("Remetente: ");
    Serial.println(telefoneSMS);
    Serial.println();

    Serial.print("Data/Hora: ");
    Serial.println(dataHoraSMS);
    Serial.println();

    Serial.println("Mensagem:");
    Serial.println(mensagemSMS);
    Serial.println();

    mensagemSMS.trim();
    if ( mensagemSMS == senhaGsm ) {

      Serial.println("Enviando SMS de Resposta.");
      leGPS();

      float flat, flon;
      unsigned long age;

      gps.f_get_position(&flat, &flon, &age);
      Serial.println("Aqui chegou!");
      /*
      Serial.println("flat: ");
      Serial.println(flat);
      Serial.println("flon: ");
      Serial.println(flon);
      Serial.println("age: ");
      Serial.println(age);
      */
      
      if ( (flat == TinyGPS::GPS_INVALID_F_ANGLE) || (flon == TinyGPS::GPS_INVALID_F_ANGLE) ) {
        enviaSMS(telefoneSMS, "GPS Sem Sinal !!!");
        Serial.println("Valores invalidos");
      } else {
        String urlMapa = "https://maps.google.com/maps/?&z=10&q=";
        urlMapa += String(flat, 6);
        urlMapa += ",";
        urlMapa += String(flon, 6);
        
        Serial.println("dentro do IF");
        Serial.println(urlMapa);
        enviaSMS(telefoneSMS, urlMapa);
      }
    }
    temSMS = false;
  }
  
}


//_______________________________________________Inicio função Configuração ______________________________________________________

void configuraGSM() {                                       //função de configuração da placa GSM com todos os comandos pra deixar o modulo habilitado para enviar e receber SMS.
  serialGSM.print("AT+CMGF=1\n;AT+CNMI=2,2,0,0,0\n;ATX4\n;AT+COLP=1\n");
  Serial.println("GSM Configurado!");
}

//_________________________________________________Inicio função GPS______________________________________________________________

void leGPS() {
Serial.println("leGPS");
unsigned long delayGPS = millis();

   serialGPS.listen();
   bool lido = false;
   while ( (millis() - delayGPS) < 500 ) { 
      while (serialGPS.available()) {
          char cIn = serialGPS.read(); 
          lido = gps.encode(cIn); 
      }

      if (lido) { 
         float flat, flon;
         unsigned long age;
    
         gps.f_get_position(&flat, &flon, &age);
    
         String urlMapa = "https://maps.google.com/maps/?&z=10&q=";
         urlMapa += String(flat,6);
         urlMapa += ",";
         urlMapa += String(flon,6);
         Serial.println(urlMapa);
         
         break; 
      }
   } 
}

//___________________________________________________Inicio função GSM_____________________________________________________________

void leGSM(){

  static String textoRec = "";                          //variavel utilizada para armazenar o texto do "SMS" recebido.
  static unsigned long delay1 = 0;                      //variavel utilizada para controle do tempo.
  static int count = 0;                                 //variavel utilizada para controlar o buffer.
  static unsigned char buffer[64];                      //buffer para armazenar dados.

  serialGSM.listen();                                   //forçando o arduino ler apartir deste momento a serial do modulo SIM800L.
  
  if (serialGSM.available()) {                          //se a serial estiver ativa faça.
    while (serialGSM.available()) {                    //enquando a serial estiver ativa execute.
      buffer[count++] = serialGSM.read();             //bufferizando dados lidos na serial GSM.
      if (count == 64)break;                          //se a variavel count recebeu o pacote de dados saia do enquanto.
    }
    textoRec += (char*)buffer;                         //transferindo dados do buffer para variavel texto recebido.
    delay1   = millis();                               //gravando tempo percorrido até esse momento na variavel delay1.

    for (int i = 0; i < count; i++) {                  //utilizando um for para limpar o buffer.
      buffer[i];
    }
    count = 0;                                         //limpando contador.
  }

  if ( ((millis() - delay1) > 100) && textoRec != "" ) {       //se o resultado do tempo até aqui menos o tempo do delay for maior que 100 e a variavel textorec conter dados faça.

    if ( textoRec.substring(2, 7) == "+CMT:" ) {      //se na posição 2,7 conter o comando "+CMT:" tem SMS recebido faça.
      temSMS = true;                                 //muda para verdadeira a variavel booleana.
    }
    
    if (temSMS) {                                     //se variavel verdadeira faça.
    //Serial.println("Tem SMS!");
      telefoneSMS = "";                              //limpando variavel.
      dataHoraSMS = "";                              //limpando variavel.
      mensagemSMS = "";                              //limpando variavel.

      byte linha = 0;
      byte aspas = 0;
      for (int nL = 1; nL < textoRec.length(); nL++) {    //for para carregar as variaveis.

        if (textoRec.charAt(nL) == '"') {
          aspas++;
          continue;
        }

        if ( (linha == 1) && (aspas == 1) ) {
          telefoneSMS += textoRec.charAt(nL);     //incrementando as variaveis com os dados correspondentes recebidos.
        }

        if ( (linha == 1) && (aspas == 5) ) {
          dataHoraSMS += textoRec.charAt(nL);     //incrementando as variaveis com os dados correspondentes recebidos.
        }

        if ( linha == 2 ) {
          mensagemSMS += textoRec.charAt(nL);     //incrementando as variaveis com os dados correspondentes recebidos.
        }

        if (textoRec.substring(nL - 1, nL + 1) == "\r\n") {
          linha++;
        }
        
      }
      
    } else {
      comandoGSM = textoRec;                         //armazenando comando recebido.
      
    }
    comandoGSM = textoRec;                         //armazenando comando recebido.
    textoRec = "";                                   //limpando variavel.
  }
}

//___________________________________________________inicio função SMS______________________________________________________________

void enviaSMS(String telefone, String mensagem) {           //função para enviar "SMS" para o telefone solicitante.
  Serial.println("Só precisa mandar mensagem!");

  serialGSM.println("AT+CMGF=1");    //Sets the GSM Module in Text Mode
  delay(1000);  // Delay of 1000 milli seconds or 1 second

  serialGSM.print("AT+CMGS=\"" + telefone + "\"\r"); // Replace x with mobile number
  delay(1000);

  serialGSM.println(mensagem);// The SMS text you want to send
  delay(100);
  
  Serial.println(mensagem);
  
   serialGSM.println((char)26);// ASCII code of CTRL+Z
  delay(100);
}
/*
---------------------------------------------------------------
    mySerial.println("AT+CMGF=1");    //Sets the GSM Module in Text Mode
  delay(1000);  // Delay of 1000 milli seconds or 1 second

  mySerial.println("AT+CMGS=\"+5511976005412\"\r"); // Replace x with mobile number
  delay(1000);

  mySerial.println("teste");// The SMS text you want to send
  delay(100);
  
   mySerial.println((char)26);// ASCII code of CTRL+Z
  delay(100);*/
