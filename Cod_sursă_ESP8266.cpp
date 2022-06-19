
#include <ESP8266WiFi.h> /////////uC+TIME
#include <ESP8266HTTPClient.h>  //AP
#include <ESP8266WebServer.h>   //AP
#include <EEPROM.h>             //AP
#include <NTPClient.h>          //TIME
#include <WiFiUdp.h>
//////////////
const char* PAROLA_MONTAJ = "2022";                  //PAROLA AUTORIZARE DISPOZITIVE
const uint8_t NR_MAX_DISP = 5;
String webpage      = "about:blank";            //Adresa refresh

String webpage_router  = "http://rotaru.onthewifi.com/";   //Adresa router
//String webpage_router = "http://192.168.0.59/";   //Adresa router

String hotspot_page = "http://192.168.43.43/";  //Adresa hotspot
String ap           = "http://192.168.4.1/";    //Adresa AP
WiFiServer server(80);                          //80 uC
ESP8266WebServer server1(80);                   //AP
WiFiUDP ntpUDP;                                 //TIME
NTPClient timeClient(ntpUDP);                   //TIME
//////////////
unsigned long currentTime = 0, previousTime = 0, Last_TIME = 0, Check_TIME = 60000, timeoutTime = 5000;
String signatures[NR_MAX_DISP];
uint8_t rooms[4], Mod_Incalzire = 0, nrdev = 0;
String name_rooms[] = { "Toate camerele", "1_DORMITOR", "2_BIROU", "3_BUC&#258;T&#258;RIE" }; // "BUCATARIE"
String s_temp_rooms[] = { "", "10.1", "10.2", "10.3" };
String s_set_temp_rooms[] = { "0.00", "17.0", "17.0", "17.0" };
String temps[] = { "17.0","17.5","18.0","18.5","19.0","19.5","20.0","20.5","21.0","21.5","22.0","22.5","23.0","23.5","24.0","24.5","25.0","25.5","26.0", "26.5", "27.0", "40.0" };
String h_rooms[] = { "", "01", "02", "03" };
uint8_t window_rooms[] = { 0,0,0,0 };
uint8_t alarm_rooms[] = { 0,0,0,0 };
uint8_t heat_rooms[] = { 0, 0, 0, 0 };
String  header = "", currentLine = "", signature = "", fromQFP = "", iii = "", oraStart = "22", oraStop = "05", minStart = "30", minStop = "50";
bool send_temp = false, newd = true, firstline = true, cookie = false, global_temp = true, Mod_Noapte = false, Status_Mod_Noapte = false;
String content, esid = "", epass = "";
uint8_t emobile = 0;

class Router {
private:
    const char* ssid = "ssid";
    const char* password = "pass";
    IPAddress* ip;
    IPAddress* gateway;//
    IPAddress* subnet;//
    IPAddress* dns1;
    bool isMobile = false;
public:
    Router() {}
    Router(const char* _ssid, const char* _password, IPAddress* _ip, IPAddress* _gateway, IPAddress* _subnet, IPAddress* _dns1, bool _isMobile) {
        ssid = _ssid;    password = _password;      ip = _ip;        gateway = _gateway;     subnet = _subnet;    dns1 = _dns1, isMobile = _isMobile;
    }
    bool gogogo() {
        WiFi.config(*ip, *gateway, *subnet, *dns1);  // 8888 sau 1111 sau 0000 sau gateway- dns pt a interoga time ok
        WiFi.begin(ssid, password);
        uint8_t too_much = 0;
        while (WiFi.status() != WL_CONNECTED && too_much < 16) { //8 sec 
            digitalWrite(LED_BUILTIN, LOW); // led on
            delay(250);
            digitalWrite(LED_BUILTIN, HIGH); // led off
            delay(250);
            too_much++;
        }
        if (WiFi.status() == WL_CONNECTED) {
            if (isMobile)
                webpage = hotspot_page;
            else
                webpage = webpage_router;
            return 1;
        }
        else 
            return 0;
    }
    const char* getSsid() {
        return ssid;
    }
};

Router* router[] = {    //Router( _ssid,   _password, * _ip, * _gateway, * _subnet, *_ddns1, isMobile) 
    new  Router("",               "",    new IPAddress(192, 168, 0, 59),  new IPAddress(192, 168, 0, 1),  new IPAddress(255, 255, 255, 0), new IPAddress(8, 8, 8, 8), false),
    new  Router("Rotaru", "Rotaru2022",   new IPAddress(192, 168, 43, 43), new IPAddress(192, 168, 43, 1), new IPAddress(255, 255, 255, 0), new IPAddress(192, 168, 43, 1), true), //mobile
    //new  Router("UPCE3E4BF6", "xz22zvehmtQm", new IPAddress(192, 168, 0, 59), new IPAddress(192, 168, 0, 1), new IPAddress(255, 255, 255, 0), new IPAddress(8, 8, 8, 8), false)
    //new  Router("Tenda_2DCA80", "mihaiesmecher12",    new IPAddress(192, 168, 0, 59), new IPAddress(192, 168, 0, 1), new IPAddress(255, 255, 255, 0), false)
};



void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH); // off
    EEPROM.begin(512); //Initialasing EEPROM
    delay(100);
    read_eeprom(); // => esid si epass si daca e hotspot mobil
    if (emobile) 
        router[0] = new  Router(esid.c_str(), epass.c_str(), new IPAddress(192, 168, 43, 43), new IPAddress(192, 168, 43, 1), new IPAddress(255, 255, 255, 0), new IPAddress(192, 168, 43, 1), true);
    else 
        router[0] = new  Router(esid.c_str(), epass.c_str(), new IPAddress(192, 168, 0, 59), new IPAddress(192, 168, 0, 1), new IPAddress(255, 255, 255, 0), new IPAddress(8, 8, 8, 8), false);

    WiFi.mode(WIFI_STA);
    uint8_t index_r = 0;
    while (WiFi.status() != WL_CONNECTED) {
        if (index_r == sizeof(router) / sizeof(Router*) ) {
            hai_eeprom();  //ca nu a gasit nimic din ce stia
        }
        for (index_r = 0; index_r < sizeof(router) / sizeof(Router*); index_r++) {
            if (strlen(router[index_r]->getSsid()) > 0) {
                if (router[index_r]->gogogo()) {
                    break;
                }
            }
        }
    }
    Serial.begin(9600);  // pornesc abia acum dup[ conectare conexiunea seriala pt ca pana acum am folosit tx(GPOI1) pentru a controla ledul albastru
    
    timeClient.setTimeOffset(10800);
    timeClient.begin();
    timeClient.update();
    server.begin();      // pornire server ... daca a ajuns aici inseamna ca s-a conectat
}

void read_eeprom() {
    for (uint8_t i = 0; i < 32; ++i) 
        esid += char(EEPROM.read(i)); //reading ssid from eeprom char by char
    for (uint8_t i = 32; i < 95; ++i) 
        epass += char(EEPROM.read(i));
    emobile = char(EEPROM.read(95));
}

void hai_eeprom() {                                 // incepe procedura de memorare a unei retele wifi fara hard coding, cu ajutorul memoriei eeprom a modulului
    WiFi.mode(WIFI_AP);                             // trec modulul in modul AccessPoint  ca sa ma pot conecta la EL prin wifi. actioneaza ca un router.
    server1.on("/", []() {                          // pagina principala fara sufix a gateway-ului  modulului
        int n = WiFi.scanNetworks();                // caut retele wireless chir daca sunt ]n modul router ap, mai exact    n = nr retele disponibile
        content = " <!DOCTYPE HTML>\r\n <html>   <head>  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"> <link rel=\"icon\" href=\"data:,\"> \r\n";
        content += "<style> html{font-family:Helvetica;display:inline-block;margin:0px auto;text-align:center;font-size 22px;}  ";
        content += "input{border-radius:15px;} input[type=checkbox]{transform: scale(1.5);} button{border-radius:15px;height:40px;background-color:ivory;}</style><title>Rotaru Ioan - Licen&#355;&#259;</title></head>\r\n";
        content += " <body style=background-color:#ff9955;> \r\n <h4>Conectarea modulului Wi-Fi la internet</h4> <hr> <form method='post' action='ssid_pass_mob'> \r\n";
        content += "<p>SSID:  <SELECT style=border-radius:15px;height:40px;text-align-last:center;background-color:ivory; name='ssid'> \r\n";
        content += "<OPTION selected=yes  disabled selected> Selecteaz&#259; numele re&#355;elei... </option> \r\n";
        for (int i = 0; i < n; i++)           
            content += "<OPTION VALUE=" + WiFi.SSID(i) + ">" + WiFi.SSID(i) + "</OPTION> \r\n";                // adaug in lista toate retelele wi-fi disponibile pe care le vede modulul
        content += "</SELECT> </p>";  //se termina par cu label_ssid + lista
        content += " <p> PASS: <input  style=width:120px;height:35px;background-color:ivory; type='password' name='pass' placeholder='parola wifi...'> </p> \r\n";  
        content += "<p> Hotspot mobil  <input  type='checkbox' name='mobile' value=\"1\"> </p>  \r\n";
        content += " <p> <input type='submit' style=height:40px;background-color:ivory; value = \"Conectare\"> </p> </form> <hr> \r\n";
        content += "<p> <a href='/'> <button type='button' >Scaneaza din nou...</button> </a> </p> ";
        content += "</body></html>";                 // content a primit continutul paginii gateway
        server1.send(200, "text/html", content);    //trimit pagina compusa mai devreme la statia conectata ( telefon conectat la ap de ex )
    });

    server1.on("/ssid_pass_mob", []() {                 //gestionez ce se intampla daca sta'ia conectata (tel) a dat submit la niste credidentials
        String qsid = server1.arg("ssid");
        String qpass = server1.arg("pass");         // salvez argumentele trimie prin submit: ssid, parola, flag daca reteaua e hotspot mobil
        uint8_t qmobile = server1.arg("mobile").toInt();  
        bool trywifi = false;                       // flag daca incerc sa ma conectez la reteaua trimisa prin submit, intai o verific
        if (qsid.length() > 0) {                    //
            trywifi = true;
            for (int i = 0; i < 96; ++i) 
                EEPROM.write(i, 0);             //stergere eeprom 
            for (int i = 0; i < qsid.length(); ++i) 
                EEPROM.write(i, qsid[i]);       //scriere in eeprom caracter cu caracter
            for (int i = 0; i < qpass.length(); ++i) 
                EEPROM.write(32 + i, qpass[i]); //scriere in eeprom caracter cu caracter
            EEPROM.write(95, qmobile);
            EEPROM.commit();
            content = "<!DOCTYPE HTML>\r\n<html><body style=background-color:#ff9955;> <h3>Modulul se conecteaz&#259; la re&#355;eaua selectat&#259; . . . </h3><hr></body</html>";
        }
        else 
            content = "<!DOCTYPE HTML>\r\n<html><body style=background-color:#ff9955;> <h3>Incearc&#259; din nou...</h3><hr></body</html>";
        server1.send(200, "text/html", content);
        delay(1000); //se reseta f[r[ s[ afi;ese pag
        if (trywifi) 
            ESP.reset();
    });

    server1.begin();
    WiFi.softAP("LICENTA_ROTARU", "licenta22"); //ssid: LICENTA_Rotaru / L_R   pass: licenta22
    digitalWrite(LED_BUILTIN, LOW);   // led ON during AP ON
    while (1) {
        server1.handleClient(); // e atent daca se conecteaza cineva la 1.4(ap)
    }

}
///////////////////////////

////////////////////////////



void loop() {
    if (Serial.available() > 0) {           // daca uC a trimis ceva (bloc standard) 
        fromQFP = Serial.readString();      // salvez ce a trimis si procesez separat
        h_rooms[1] = fromQFP.substring(0, 2);
        h_rooms[2] = fromQFP.substring(2, 4);
        h_rooms[3] = fromQFP.substring(4, 6);
        s_temp_rooms[1] = fromQFP.substring(6, 10);
        s_temp_rooms[2] = fromQFP.substring(10, 14);
        s_temp_rooms[3] = fromQFP.substring(14, 18);
        heat_rooms[0] = fromQFP.substring(18, 19).toInt();
        heat_rooms[1] = fromQFP.substring(19, 20).toInt();
        heat_rooms[2] = fromQFP.substring(20, 21).toInt();
        heat_rooms[3] = fromQFP.substring(21, 22).toInt();
        window_rooms[1] = (fromQFP.substring(22, 23)).toInt();
        window_rooms[2] = (fromQFP.substring(23, 24)).toInt();
        window_rooms[3] = (fromQFP.substring(24, 25)).toInt();
        alarm_rooms[1] = (fromQFP.substring(25, 26)).toInt();
        alarm_rooms[2] = (fromQFP.substring(26, 27)).toInt();
        alarm_rooms[3] = (fromQFP.substring(27, 28)).toInt();
    }

    WiFiClient client = server.available();     //creez obiect de tip client 
    client.setTimeout(100);                     // ii setez timeout la 100ms ca nu dureaza mai mult sa trimit[ textul cu req
    if (client) {                               //daca am un client conectat
        currentTime = millis();                 //salvez timpul curent
        previousTime = currentTime;             //salvez timpul curent
        while (client.connected() && currentTime - previousTime <= timeoutTime) {     //cat timp am client conectat si nu sta conectat degeaba
            currentTime = millis();                                                   //gestionez cat timp am clientul conectat
            if (client.available()) {                                                 //daca clientul a trimis un req
                if (firstline) {                                                      //    daca e prima linie din req
                    header = client.readStringUntil('\n');                            //    salveaz-o pana la final
                    //Serial.println(header);////////////////////////////////////////////////////////
                    firstline = false;                                                //    reset flag de linia1 ( e importanta primalinie)
                }
                currentLine = client.readStringUntil('\r');                           //citec linie cu linie requestul clientului(am interes pt tot blocul)
                client.read();                                                        //trec peste caracterul newline ca sa nu il salvez in linia curenta
                if (currentLine.startsWith("User-Agent:")) {                          //(linia cu semnatura disp)
                    signature = currentLine.substring(12);                            //salvez temporar semnatura disp care a facut req
                    for (uint8_t k = 0; k <= NR_MAX_DISP; k++) {                      //caut sa vad daca cunosc disp ce a facut req
                        if (signatures[k] == signature) {                             //  daca cunosc dispozitivul
                            if (header.startsWith("GET /?room="))                     //    dac[ vrea s[ vad[ param dintr-o camer[
                                rooms[k] = (header.substring(11, 12)).toInt();        //      salveaz[ camera pe care o vrea
                            if (header.startsWith("GET /?tx=")) {                     //    daca vrea sa sch t in cam la care se uita
                                s_set_temp_rooms[rooms[k]] = header.substring(9, 13); //      salveaz[ t pe care o vrea in cam la care se uita
                                if (rooms[k] == 0) {                                  //      daca se uit[ la "toate camerele"
                                    global_temp = true;                               //        inseamna ca vrea sa schimbe t in toate camerele
                                    for (uint8_t i = 1; i <= 3; i++)                  //        parcurg camerele 1,2,3
                                        s_set_temp_rooms[i] = s_set_temp_rooms[0];    //          salvez t dorita pentru toate cam
                                }
                                else                                                  //      altfel daca se uita la o singura cam
                                    global_temp = false;                              //          seteaza flag de temp globala off 
                                send_temp = true;                                     //    seteaz[ flag ca sa trimita noile temp catre uC
                                cookie = true;                                        //    seteaz[ flag pt refresh web mai rapid ca sa se vada
                            }                                                                    // modificarile procesate de uC trimise inapoi

                            if (header.startsWith("GET /?M=")) {                       //    dac[ vrea s[ schimbe modul de incalzire
                                Mod_Incalzire = (header.substring(8, 9)).toInt();                 //      salveaz[ modul dorit
                                send_temp = true;                                       //      flag ca sa trimita modul catre uC
                            }                       /// ?T=1&T=0&N=20%3A00&D=06%3A00
                                                    //      ?T=0&N=20%3A00&D=06%3A00
                            if (header.startsWith("GET /?T=")) {                        //    dac[ vrea s[ schimbe modul noapte
                                send_temp = true;                                       //      trimite temp din nou la uC
                                if (header.substring(8, 9) == "1") {                   //      daca vrea sa il opreasca
                                    Mod_Noapte = true;
                                    oraStart = (header.substring(16, 18));
                                    minStart = (header.substring(21, 23));
                                    oraStop = (header.substring(26, 28));
                                    minStop = (header.substring(31, 33));
                                }
                                else {//daca e 0
                                    Mod_Noapte = false;
                                    oraStart = (header.substring(12, 14));
                                    minStart = (header.substring(17, 19));
                                    oraStop = (header.substring(22, 24));
                                    minStop = (header.substring(27, 29));
                                }
                                set_Status_Mod_Noapte();
                            }


                            SHOW(client, rooms[k], cookie);                           //  trimite pagina web clientului cu modificarile 
                            cookie = false;                                           //  reseteaz[ flag pt refresh rapid 
                            newd = false;                                             //  (flag)cunosc disp deci nu e un newDevice
                            if (send_temp) {                                          //  daca am flag de trimitere temp ON 
                                Serial.print(s_set_temp_rooms[1] + s_set_temp_rooms[2] + s_set_temp_rooms[3] + Mod_Incalzire + Status_Mod_Noapte);  // trimite temp la uC
                                send_temp = false;                                    //  setez flag off dupa ce le-am trimis
                            }
                            break;                                                    //  cunosc disp, nu mai caut altul cu aceeasi semnatura
                        }//daca cunosc disp
                    }//for: caut disp daca e autorizat
                }//linia cu semnatura
                if (currentLine.length() == 0) {                                      //daca linia la care am ajuns e goal[ inseamna ca sunt la finalul blocului de req
                    if (newd) {                                                       //  daca nu cunosc disp care a facut req
                        String post_data = client.readString();                       //    citesc linia de dupa linia goala ce poate contine o parola
                        //Serial.println(currentLine);////////////////////////////////////////////////////////
                        //Serial.println(post_data);////////////////////////////////////////////////////////
                        bool first_try = true;                                        //    consider ca a prima data cand baga parola
                        bool nokpass = false;                                         //    consider ca o introduce gresit (flag)
                        if (post_data.substring(0, 9) == "password=") {               //    daca disp incearc[ sa se autorizeze
                            if (post_data.substring(9) == PAROLA_MONTAJ) {             //        daca a introdus parola CORECT
                                signatures[nrdev] = signature;                        //            AUTORIZARE OK
                                first_try = false;                                        //            reset flag prima incercare
                                SHOW(client, rooms[nrdev], false);//////////////////////            trimit catre el pagina cu camerele
                                nrdev++;                                              //            incrementez nr disp cunoscute
                            }
                            else                                                      //        daca a introdus parola GRESIT
                                nokpass = true;                                       //            flag parola gresit[
                        }
                        if (first_try)                                                //    daca e prima daca c[nd acceseaza sau daca a gresit parola  
                            SHOW_PASS(client, nokpass);/////////////////////////////////        ii arat pagina de conectare (cu mesaj de parola gresita daca e cazul)
                    }
                    break;
                }
                //Serial.println(currentLine);////////////////////////////////////////////////////////
            }
        }
        newd = true;
        firstline = true;
        client.stop();
    }
    
    if (Mod_Noapte) {
        currentTime = millis();
        if (currentTime - Last_TIME > Check_TIME) {  //Odat[ la Check_TIME verific ceasul prin internet
            Last_TIME = currentTime;
            set_Status_Mod_Noapte();
            Serial.print(s_set_temp_rooms[1] + s_set_temp_rooms[2] + s_set_temp_rooms[3] + Mod_Incalzire + Status_Mod_Noapte);
        }
    }

}

void SHOW(WiFiClient& client, uint8_t& room, bool  cookie) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println("Connection: close");
    client.println();                                               // dupa header necesar pt browser trimit pag html
    client.println("<!DOCTYPE html><html>");
    client.println("<head><meta name='viewport' content='width=device-width, initial-scale=1'>");

    if (cookie)                                                     // flag refresh rapid
        client.println("<meta http-equiv=refresh content='7; url=" + webpage + "'/>");
    else 
        if(room)
            client.println("<meta http-equiv=refresh content='14; url=" + webpage + "'/>");

    client.println("<link rel=\"icon\" href=\"data:,\">");         //iconita pt browser
    client.println("<style>html{font-family:Helvetica;display inline-block;margin:0px auto;text-align:center;font-size:18px;}");
    client.println("table{margin-left:auto;margin-right:auto;border-spacing:5px 10px;} tr{height:35px;} th,td{border-radius:15px;}");
    client.println("tr:nth-child(even){background-color:rgba(255,255,255,0.05);} tr:nth-child(odd){background-color:rgba(255,255,255,0.2);}</style>");     
    client.println("<title>Rotaru Ioan - Licen&#355;&#259;</title> </head> <body style=background-color:#ff9955;> <h3> Control &#238;nc&#259;lzire</h3> <hr>");
    if (heat_rooms[0])
        client.println("<table><td style=background-color:GoldenRod;>Centrala termic&#259; este PORNIT&#258;</td> </table>");
    else 
        client.println("<table><td style=background-color:DarkTurquoise;>Centrala termic&#259; este OPRIT&#258;</td> </table>");

    client.print("<hr><form  method=get>  <p><b>Camera: &nbsp&nbsp </b> ");
    client.println("<SELECT onchange=this.form.submit() NAME=room style=width:140px;height:35px;background-color:ivory;border-radius:15px;text-align-last:center;>");
    for (uint8_t r = 0; r <= 3; r++) {
        client.print("<OPTION ");
        if (r == room) 
            client.print("selected=yes ");
        client.print("VALUE=");
        client.print(r);
        client.println(">" + name_rooms[r] + "</OPTION>");
    }
    client.println("</SELECT></p></form>");

    if (room > 0) {
        client.println("<p> Temperatura actual&#259;: <b>" + s_temp_rooms[room] + " &#8451; </b></p>");
        client.println("<form  method=get> <p><b>Temperatura dorit&#259;: &nbsp </b>");
        client.println("<SELECT onchange=this.form.submit() NAME=tx style=width:70px;height:35px;background-color:ivory;border-radius:15px;text-align-last:center;>");
        for (String iiii : temps) {
            client.print("<OPTION ");
            if (iiii == s_set_temp_rooms[room]) 
                client.print("selected=yes ");
            client.println("VALUE=" + iiii + ">" + iiii + "</OPTION>");
        }
        client.println("</SELECT> &#8451;</p></form>");

        client.println("<table><tr><td style=width:180px;>Umiditatea:</td><th style=width:120px;>" + h_rooms[room] + " % </th></tr>");
        (heat_rooms[room]) ? client.print("<tr><td>Robinet calorifer:</td><th style=background-color:GoldenRod;>Deschis</th></tr>") : client.print("<tr><td>Robinet calorifer:</td><th style=background-color:DarkTurquoise;>Inchis</th></tr>");
        (window_rooms[room]) ? client.println("<td>Starea Ferestrei:</td><th style=background-color:tomato;>Deschis&#259;</th></tr>") : client.println("<tr><td>Starea Ferestrei:</td><th>&#206;nchis&#259;</th></tr>");
        (alarm_rooms[room]) ? client.println("<tr><td>Alarm&#259; sonor&#259;:</td><th style=background-color:tomato;>Pornit&#259;</th></tr></table>") : client.println("<tr><td>Alarm&#259; sonor&#259;:</td><th>Oprit&#259;</th></tr></table>");
    }
    if (room == 0) {
        client.println("<table><tr><form method=get> <td>Seta&#355;i temperatura<br> in toate camerele: &nbsp </td>");
        client.println("<td><SELECT onchange=this.form.submit() NAME=tx style=width:px;height:35px;background-color:ivory;border-radius:15px;text-align-last:center;>");
        if (!global_temp) {
            client.print("<option value='' disabled selected>Setat&#259; particular...</option>");
            for (String iiii : temps)
                client.print("<OPTION VALUE=" + iiii + ">" + iiii + "</OPTION>");
        }
        else {
            for (String iiii : temps) {
                client.print("<OPTION ");
                if (iiii == s_set_temp_rooms[room])
                    client.print("selected=yes ");
                client.println("VALUE=" + iiii + ">" + iiii + "</OPTION>");
            }
        }
        if (!global_temp) 
            client.println("</SELECT></td></form></tr>");
        else
            client.println("</SELECT> &#8451;</td></form></tr>");

        client.print("<tr><form method=get><td>Mod noapte :<input style=width:20px;height:20px; type='checkbox' name='T' value='1'");
        if (Mod_Noapte)
            client.print(" checked");
        client.print("/><input type='hidden' name='T' value='0'/> <br> <input style=background-color:ivory;border-radius:15px;height:30px; type='submit' value='Salveaza'> </td>");
        client.print("<td align='right'><label>T<sub>Start</sub> =</label><input style=width:70px;height:30px; type='time' name='N' min='12:00' max='23:59' value='"); 
        client.print( oraStart + ":" + minStart ); // timp start
        client.print("'><br><label>T<sub>Stop</sub> =</label><input style=width:70px;height:30px; type='time' name='D' min='00:00' max='11:59', value='");
        client.print( oraStop + ":" + minStop ); //timp stop"
        client.print("'></td></form></tr>");

        client.println("<tr><form method=get><td>Mod &#206;nc&#259;lzire:</td><td align='left'>");
        client.println("<input style=width:20px;height:20px; onclick=this.form.submit() type='radio' id='normal' name='M' value='0'");
        if (Mod_Incalzire == 0)
            client.println(" checked ");
        client.println("> <label for='normal'>Mod Normal</label><br> <input style=width:20px;height:20px; onclick=this.form.submit() type='radio' id='confort' name='M' value='1'");
        if (Mod_Incalzire == 1)
            client.println(" checked ");
        client.println("> <label for='confort'>Mod Confort</label><br> <input style=width:20px;height:20px; onclick=this.form.submit() type='radio' id='eco' name='M' value='2'");
        if (Mod_Incalzire == 2)
            client.println(" checked ");
        client.println("> <label for='eco'>Mod Eco</label><br></td></form></tr>");

        if (heat_rooms[0]) {
            client.println("<tr> <td>&#206;nc&#226;lzirea func&#355;ioneaz&#259; &#238;n:</td> <th> <ul align='left'>");
            for (uint8_t i = 1; i <= 3; i++) {
                if (heat_rooms[i])
                    client.println("<li>" + name_rooms[i] + "</li>");
            }
            client.println("</ul></th></tr>");
        }
        client.println("</table>");
    }
    client.println("</body></html>");
    client.println();
}

void SHOW_PASS(WiFiClient& client, bool nok) {  // afisare pagina de autorizare 
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println("Connection: close");
    client.println();       //dupa acest header necesar pt browser se trimite si pagina html
    client.println("<!DOCTYPE html><html>");
    client.println("<head><meta name='viewport' content='width=device-width,initial-scale=1'>");
    client.println("<link rel=\"icon\" href=\"data:,\">");
    client.println("<style>html{font-family:Helvetica;display:inline-block;margin:0px auto;text-align:center;font-size:22px;}");
    client.println("</style>");
    client.println("<title> Rotaru Ioan - Licenta </title>  ");
    client.println("</head>  <body style=background-color:#ff9955;><h2>Control Ambiental</h2><hr>");
    client.println("<form method=post><input type=password style=width:150px;border-radius:15px;height:35px; name='password' id='password' placeholder='Introduceti parola...'> ");
    client.println("<input type='submit' style=height:40px;background-color:ivory;border-radius:15px; value='Autorizare dispozitiv'> </form>");
    if (nok)     // flag daca noul disp a gresit parola afiseaza mesaj
        client.println("<p style=color:red;> Parola gresita :( </p>");
    client.println("<hr> <hr>  </body> </html>");
    client.println();
}


void set_Status_Mod_Noapte() {
    Status_Mod_Noapte = false;          //consider ca nu trebuie activat, dupa il activez daca se indeplinesc conditiile
    if (Mod_Noapte) {                   //daca modul noapte e activ, verific daca trebuie 
        timeClient.update();
        uint8_t HH = timeClient.getHours();   
        uint8_t MM = timeClient.getMinutes();
            //12:01-23:59 (Start->PM)    // 00:01 - 11:59 (Stop->AM)
        if ( HH >= 0  &&  HH <= 11 ) {           //daca e dimi 
            if ( HH < oraStop.toInt() )         //daca ora curenta e sub ora de stop inseamna ca pornesc modul noapte
                Status_Mod_Noapte = true;
            else 
                if ( HH == oraStop.toInt()   &&   MM <= minStop.toInt() )
                    Status_Mod_Noapte = true;
        }
        else 
            if (HH >= 12 && HH <= 23) {               //daca e seara 
                if (HH > oraStart.toInt() )           //daca ora curenta e peste ora de start inseamna ca pornesc modul noapte
                    Status_Mod_Noapte = true;
                else
                    if (HH == oraStart.toInt() && MM >= minStart.toInt() )
                        Status_Mod_Noapte = true;
            }
    }     
}
