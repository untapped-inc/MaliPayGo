void postData(String body){
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
     }else{
      Serial.println("Error connecting to server");
     }
  }
}

//check for the response to the HTTP POST
void checkResponse(){
  //hold the JSON results
  String deviceString;
  //find the nested objects using this
  int bracketCount = 0;
  //read from the API response
   while (client.available()) {
     //Serial.println("checking response");
    char c = (char)client.read();
    Serial.print(c);
   }
}

    /*
    //wait for the JSON
    if (c == '{'){
      bracketCount++;
    }
    //start building the response string
    if (bracketCount == 2){
      deviceString += c;
    }else if (bracketCount == 3){
      maxWaterString +=c;
    }
  }
  if (maxWaterString != "" && deviceString != ""){
    //Serial.print("response string: " + responseString);
    //deserialize
    DeserializationError error1 = deserializeJson(jsonDoc1, deviceString);
    DeserializationError error2 = deserializeJson(jsonDoc2, maxWaterString);
    long currentWaterChar = jsonDoc1["current_water_amount"];
    long maxWaterChar = jsonDoc2["value"];
    if (currentWaterChar > 0){
      //set the water variable
      currentWaterAmount = currentWaterChar;
      Serial.print("currrent water char ");
      Serial.println(currentWaterChar);
    }
  
    if(maxWaterChar > 0){
      //set the max water variable
      maxWaterAmount = maxWaterChar;
      Serial.print("max water char ");
      Serial.println(maxWaterChar);
    }
  }
}
*/
