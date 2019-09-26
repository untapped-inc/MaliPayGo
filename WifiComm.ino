void getData(){
  if(WiFi.status()== WL_CONNECTED){   //Check WiFi connection status

   if (client.connect(URL, 80)){
    Serial.println("Getting current water");
      client.print("GET /sema/water-ops/pgwc/");
      client.print(DEVICE_ID);
      client.println(" HTTP/1.1"); 
      client.print("Host: ");
      client.println(URL);
      client.println("Content-type:application/json");
      client.println("Connection: close");
      client.println();
   }else{
    Serial.println("Error connecting to server");
   }
  }
}


void postData(String body){
  if(WiFi.status()== WL_CONNECTED){   //Check WiFi connection status

     if (client.connect(URL, 80)){
        Serial.println("Body");
        Serial.println(body);
        client.println("POST /sema/water-ops/pgwc HTTP/1.1"); 
        client.print("Host: ");
        client.println(URL);
        client.println("Content-type: application/json");
        client.println("Accept: */*");
        client.print("Host ");
        client.println(URL);
        client.println("accept-encoding: gzip, deflate");
        client.print("content-length: ");
        client.println(body.length());
        //see https://blog.insightdatascience.com/learning-about-the-http-connection-keep-alive-header-7ebe0efa209d
        client.println("Connection: close");
        client.println();
        client.println(body);
        client.println();
     }else{
      Serial.println("Error connecting to server");
     }
  }

}
