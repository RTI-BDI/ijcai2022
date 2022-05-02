using System.Collections;
using System.Collections.Generic;
using System.Net.Sockets;
using System.Net;
using System.Text;
using System;
using UnityEngine;

public class Client
{
	const int PORT_NO = 8080;
	const string SERVER_IP = "127.0.0.1";
	private Socket sender;
	
	public Client() { }
	
	public void Connect(){
	
		try{
			// Establish the remote endpoint for the socket.  
            // This example uses port 8080 on the local computer.  
            IPHostEntry ipHostInfo = Dns.GetHostEntry(Dns.GetHostName());  
            IPAddress ipAddress = ipHostInfo.AddressList[0];  
            IPEndPoint remoteEP = new IPEndPoint(ipAddress, PORT_NO);  
  
            // Create a TCP/IP  socket.  
            this.sender = new Socket(ipAddress.AddressFamily,
                SocketType.Stream, ProtocolType.Tcp );  
                
            sender.ReceiveTimeout = 5000;
            sender.Connect(remoteEP);
            Debug.Log("Socket connected to " +  sender.RemoteEndPoint.ToString());  
                
		} catch (Exception e) {  
            Debug.Log( e.ToString());  
        }
	}
	
	public string SendMessage(string message){
		// Data buffer for incoming data.  
        byte[] bytes = new byte[1024*150];
        //byte[] msg = new byte[1024*100];  
  
            // Connect the socket to the remote endpoint. Catch any errors.  
            try {  
                
                message = message + "end_json";
                
                // Encode the data string into a byte array.  
                byte[] msg = Encoding.ASCII.GetBytes(message);  
  
                // Send the data through the socket.  
                int bytesSent = sender.Send(msg);  
  
                // Receive the response from the remote device.  
                int bytesRec = sender.Receive(bytes);  
                Debug.Log("Echoed test = " +   
                    Encoding.ASCII.GetString(bytes,0,bytesRec));  
  
                // Release the socket.  
                //sender.Shutdown(SocketShutdown.Both);  
                //sender.Close();

				return Encoding.ASCII.GetString(bytes, 0, bytesRec);


			} catch (ArgumentNullException ane) {  
                Debug.Log("ArgumentNullException : " + ane.ToString());  
            } catch (SocketException se) {  
                Debug.Log("SocketException : " + se.ToString());  
            } catch (Exception e) {  
                Debug.Log("Unexpected exception : " + e.ToString());  
            }  

		return "Error";
	}

	public string SendRun(int frame)
	{
		string message = "{ \"command\" : \"RUN\", \"time\" : " + frame + " }";
		return SendMessage(message);
	}

	public string SendPoke()
	{
		string message = "{ \"command\" : \"POKE\" }";
		return SendMessage(message);
	}

	public string SendUpdate(string fileName, string content)
	{
		string message = "{ \"command\" : \"UPDATE\", \"file\" : \"" + fileName + "\", \"content\" : " + content + " }";
		Debug.Log(message.Length);
		return SendMessage(message);
	}

	public string SendInitialize(string fileName, string content)
	{
		string message = "{ \"command\" : \"UTIL\", \"request\" : \"INITIALIZE\", \"file\" : \"" + fileName + "\", \"content\" : " + content + " }";
		return SendMessage(message);
	}
	
	public string SendSetupCompleted(){
		string message = "{ \"command\" : \"SETUP_COMPLETED\" }";
		return SendMessage(message);
	}

	public string SendExit() {
		string message = "{ \"command\" : \"UTIL\", \"request\" : \"EXIT\" }";
		return SendMessage(message);
	}
}
