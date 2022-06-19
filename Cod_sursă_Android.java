package com.example.rotaru_ioan_licenta22;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.os.Bundle;
import android.view.View;
import android.webkit.WebResourceError;
import android.webkit.WebResourceRequest;
import android.webkit.WebView;
import android.webkit.WebViewClient;
//import android.widget.Button;
import android.widget.ImageButton;
import android.widget.ProgressBar;

import androidx.appcompat.app.AppCompatActivity;





public class MainActivity extends AppCompatActivity {



    String webpage =      "http://rotaru.onthewifi.com/"; //router home
    String webpage1 =      "http://192.168.0.59/";        //router home
    String hotspot_page = "http://192.168.43.43/";        //mobile hotspot
    String ap_page =      "http://192.168.4.1/";          //access point

    WebView webView;


    @SuppressLint("SetJavaScriptEnabled")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        ProgressBar progressBar = findViewById(R.id.progressBar);

        webView = findViewById(R.id.webView);
        webView.setWebViewClient(new WebViewClient() {
            @Override
            public void onReceivedError(WebView view, WebResourceRequest request, WebResourceError error){
                String content = "<!DOCTYPE html><html>\n" +
                    "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n" +
                    //"<meta http-equiv=refresh content=\"5; url=" + webpage + "\"/>\n" +
                    "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center; font-size: 20px;}</style></head>\n" +
                    "<body style=background-color:#ff9955;> <h2>Eroare de conectare</h2> <hr>\n" +
                    "<p>Verifica&#355;i re&#355;eaua &#351;i montajul, apoi <a href=" + webpage + "> <button type='button' style=background-color:ivory;height:30;border-radius:15px;  >Re&#238;ncerca&#355;i...</button></a></p>" +
                    "</body><html>";
                webView.loadDataWithBaseURL(null, content, "text/html", "UTF-8", null);
            }

//            @Override
//            public void onPageStarted(WebView view, String url, Bitmap favicon) {   //am ]nlocuit cu onLoadResource pt ca nu se invartea
//                super.onPageStarted(view, url, favicon);
//                progressBar.setVisibility(View.VISIBLE);
//            }
            @Override
            public void onLoadResource(WebView view, String url) {
                super.onLoadResource(view, url);
                progressBar.setVisibility(View.VISIBLE);
            }


            @Override
            public void onPageFinished(WebView view, String url) {
                super.onPageFinished(view, url);
                progressBar.setVisibility(View.INVISIBLE);
            }

        });
        webView.getSettings().setJavaScriptEnabled(true);
        webView.setBackgroundColor(Color.TRANSPARENT); /////
        webView.loadUrl("about:blank");
        webView.loadUrl(webpage);                           // la pornirea aplciatiei se conecteaza direct la domeniu


        ImageButton button = findViewById(R.id.button);      // refresh button
        button.setOnClickListener(v -> {                      //la apasare
            webView.loadUrl("about:blank");                   //resetare view
            webView.loadUrl(webpage);                          //se conecteaz[ la domeniu
        });

        ImageButton button2 = findViewById(R.id.button2);   // exit button
        button2.setOnClickListener(v -> {
            webView.loadUrl("about:blank");
            finish();                                       //opreste executiile
            System.exit(0);                                 //iese in aplciatie
        });

        ImageButton button3 = findViewById(R.id.button3);   //mobile hotspot 
        button3.setOnClickListener(v -> {
            webView.loadUrl("about:blank");
            webView.loadUrl(hotspot_page);                  //acceseaza sistemul dar nu de la sistant, ci fiind conectat la hotspot mobil
        });

        ImageButton button4 = findViewById(R.id.button4);
        button4.setOnClickListener(v -> {
            webView.loadUrl("about:blank");                 //scurtatura setari wifi
            startActivity(new Intent(android.provider.Settings.ACTION_WIFI_SETTINGS));
        });

        ImageButton imageButton = findViewById(R.id.imageButton);  //conectare router nou .. pagina generata de modul in modul AccesPoint
        imageButton.setOnClickListener(v -> {
            webView.loadUrl("about:blank");
            webView.loadUrl(ap_page);                             //acceseaza pagina cu formularul inc are se introduce noua adresa
        });

        progressBar.setOnClickListener(v -> {
            webView.loadUrl("about:blank");
            webView.loadUrl(webpage1);                          // conectare la sistem dar in raza routerului la care e conectat
                }
        );


    }



}





