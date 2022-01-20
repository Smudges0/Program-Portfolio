
import java.io.*;
import java.net.*;
 
public class HTTPClient {
    public static void main(String[] args) throws IOException {
         
        if (args.length != 2) {
            System.err.println(
                "Usage: java EchoClient <host name> <port number>");
            System.exit(1);
        }
 
        String hostName = args[0];
        int portNumber = Integer.parseInt(args[1]);
        Socket socket = null;
 
        while (true) {

            try {
                String fromServer;
                String fromUser;

                BufferedReader stdIn = new BufferedReader(new InputStreamReader(System.in));
                System.out.println("Request? ");
                fromUser = stdIn.readLine();
                //System.out.println("Client: " + fromUser);

                socket = new Socket(hostName, portNumber);
                PrintWriter out = new PrintWriter(socket.getOutputStream(), true);
                BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()));

                out.println(fromUser);
                out.println();

                while(true) {
                    fromServer = in.readLine();
                    if (fromServer == null || fromServer.length()==0) { break; }
                    System.out.println("Server: " + fromServer);
                }

                System.out.println("----------------");
            } catch (UnknownHostException e) {
                System.err.println("Don't know about host " + hostName);
                System.exit(1);
            } catch (IOException e) {
                System.err.println("Couldn't get I/O for the connection to " + hostName);
                System.exit(1);
            } finally {
                if (socket != null) { socket.close(); }
            }
        }    
    }
}
