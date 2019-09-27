void postDataold(String body){
  if(WiFi.status()== WL_CONNECTED){   //Check WiFi connection status

     if (client.connect(URL, 80)){
        client.println("POST /api/PGWC HTTP/1.1"); 
        client.print("Host: ");
        client.println(URL);
        client.println("Content-type: application/json");
        client.println("Accept: */*");
        client.println("Cache-Control: no-cache");
        client.println("Accept-Encoding: gzip, deflate");
        client.print("Content-Length: ");
        client.println(body.length());
        //see https://blog.insightdatascience.com/learning-about-the-http-connection-keep-alive-header-7ebe0efa209d
        client.println("Connection: close");
        client.println();
        client.println(body);

       delay(5000);
        while (client.available()) {
          char c = client.read();
          Serial.write(c);
        }
                
        client.flush();
        client.stop();
     }else{
      Serial.println("Error connecting to server");
     }
  }
}

/*
 * This was added because the body of the response to the 
 * HTTP Post was coming back garbled in the original method.
 * The Arduino HTTP Client library makes the request MUCH simpler and 
 * decodes the response body
 * 
 * Returns the "millilitersRemaining" that are reported by the response body
 */
int postData(String body){
  //here's an example for using this librayr: 
  //https://github.com/arduino-libraries/ArduinoHttpClient/blob/master/examples/CustomHeader/CustomHeader.ino
  client.beginRequest();
  client.post("/api/PGWC");
  client.sendHeader("Content-type", "application/json");
  client.sendHeader("Content-Length", body.length());  
  client.endRequest();
  client.write((const byte*)body.c_str(), body.length());


  // read the status code and body of the response
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  if (statusCode == SUCCESSFUL_RESPONSE_CODE){
    String totalMLBuffer = "";
    
    //loop through the responde body
    for(int i = 0; i < response.length();  i++){
      char responseChar = (char)response[i];
      //parse out the total milliliters value
      if (isDigit(responseChar)){
        totalMLBuffer += responseChar;
      }
    }
    //return the value
    return totalMLBuffer.toInt();
  }else{ 
    return ERROR_CODE;
  }
}
