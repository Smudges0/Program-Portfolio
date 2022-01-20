
import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.util.Map;

public class RequestHandler extends Thread {
    private Socket socket = null;
    private PrintWriter out;
    private BufferedReader in;
    private InputStream fileIn;
    private BufferedOutputStream fileOutSocket;
    private static final int SOCKET_TIMEOUT = 5000;

    // Create a Request and a Response object
    private Request request = new Request();
    private Response response = new Response();

    private Logger log;

    public RequestHandler(Socket socket) {
        super("RequestHandler");
        this.socket = socket;
        log = Logger.getLogger(this.getClass());
    }

    public void run() {
        try {
            log.info("Handling request from port " + socket.getPort());

            // Set a timeout on the socket in case nothing is returned on read for 30
            // seconds.
            socket.setSoTimeout(SOCKET_TIMEOUT);

            out = new PrintWriter(socket.getOutputStream(), true);
            in = new BufferedReader(new InputStreamReader(socket.getInputStream()));

            readFirstLine();
            // if code is set at this point, an error occured. Send error response
            if (response.isReadyToSend()) {
                sendResponse(response);
                return;
            }

            readHeader();
            if (response.isReadyToSend()) { // if code is set at this point, an error occured. Send error response
                sendResponse(response);
                return;
            }

            readBody(); // If body doesn't exist, nothing will change

            if ((request.getMethod()).equals("POST")) { // If POST, then we have to run a CGI script
                CGIhandler.invokeCGI(request, response);
            } else { // Otherwise it is a GET or HEAD post, which FileGetter.getFileInfo handles
                FileGetter.getFileInfo(request, response);
            }

            if (!(response.isReadyToSend())) { // If response is NOT ready to send, then everything has gone well. Set
                                               // response code.
                response.setCode(200);
            }

            // At this point response WILL have a code, so we can send
            sendResponse(response);
        } catch (SocketTimeoutException e) {
            response.setCode(408);
            sendResponse(response);
            return;
        } catch (IOException e) {
            e.printStackTrace();
        } catch (Exception e) {
            e.printStackTrace();
            response.setCode(500);
            sendResponse(response);
            return;
        }
    }

    private void readFirstLine() throws IOException {
        String inputLine = in.readLine(); // Read first line of the client's request
        if (inputLine == null) { // Didn't receive anything, bad request
            response.setCode(400);
            return;
        }

        String[] parts = inputLine.split(" ");
        if (parts == null || parts.length != 3) { // Missing method, URI, or http version, bad request
            response.setCode(400);
            return;
        }

        if (!Request.isMethodValid(parts[0])) { // Unknown method, bad request
            response.setCode(400);
            return;
        }
        if (!Request.isMethodSupported(parts[0])) { // Unsupported method, not implemented
            response.setCode(501);
            return;
        }
        request.setMethod(parts[0]); // Method is valid AND supported. Record method in Request object.

        // It can be sent as blank if you send this request "GET HTTP/1.0" (2 spaces.
        // Splits into 3 parts with part[1] = "")
        // Per RFC - "Note that the absolute path cannot be empty; if none is present in
        // the original URI, it must be given as "/" (the server root)."
        // https://tools.ietf.org/html/rfc1945#section-5.1.2
        // TODO - Need to URLDecode the URI....
        request.setURI(parts[1]); // URI OK, record URI in Request object

        String[] version = parts[2].split("/");
        if (version.length != 2 || !version[0].equals("HTTP")) { // Version does not read 'HTTP', bad request
            response.setCode(400);
        }
        if (!version[1].equals("1.0")) { // Version number does not match, HTTP version not suported
            response.setCode(505);
        }
        request.setHttpVersion(parts[2]); // HTTP version OK, record in Request object.
    }

    private void readHeader() throws IOException {
        String headerLine;
        while ((headerLine = in.readLine()).length() > 0) {
            String[] parts = headerLine.split(": ");
            if (parts.length != 2) { // Might be the body segment
                response.setCode(400);
                return;
            }
            request.setHeader(parts[0], parts[1]);
        }
    }

    private void readBody() throws IOException {
        String headerLine;
        StringBuilder query = new StringBuilder(request.getBody());
        while ((headerLine = in.readLine()).length() > 0) {
            query.append(headerLine);
            query.append("\n");
        }
        query.setLength(query.length() - 1);
        request.setBody(query.toString());
    }

    private void sendResponse(Response response) {
        int code = response.getCode();
        log.debug("Sending " + code + " to port " + socket.getPort());
        try {
            out.print(Response.HTTP_VERSION);
            out.print(" ");
            out.print(code);
            out.print(" ");
            out.print(Response.getCodeMessage(code));
            // Use print("\r\n")+flush() instead of println(). println() does not work with
            // the HTTPServerTester on Linux, but works on Windows!!!!!
            // See System.lineSeparator() javadoc: On UNIX systems, it returns "\n"; on
            // Microsoft Windows systems it returns "\r\n".
            // HTTPServerTester is hardcoded to split the server response using "\r\n", even
            // on Linux, so all the headers remain in a single string!
            out.print("\r\n");
            out.flush();
            // out.println();

            log.debug(Response.HTTP_VERSION + " " + code + " " + Response.getCodeMessage(code));

            if (code == 200) {
                // Send header + body to client.

                for (Map.Entry<String, String> entry : response.getHeaderEntries()) {
                    out.print(entry.getKey());
                    out.print(": ");
                    out.print(entry.getValue());
                    out.print("\r\n");
                    // out.println();
                }
                out.print("\r\n");
                out.flush();
                // out.println();

                // Send Body
                // Must flush() before sending the body. Binary files may overwrite the
                // previously printed headers if you don't flush first.
                if (!request.getMethod().equals("HEAD")) {
                    // Can't use the PrintWriter for sending file data. Need to write bytes.
                    fileIn = new BufferedInputStream(new FileInputStream(response.getRequestedFile()));
                    fileOutSocket = new BufferedOutputStream(socket.getOutputStream());

                    byte[] buffer = new byte[8192];
                    int bytesRead;
                    while ((bytesRead = fileIn.read(buffer)) > 0) {
                        fileOutSocket.write(buffer, 0, bytesRead);
                    }
                }
            } else if (code == 304) {
                out.print("Expires: " + response.getHeader("Expires"));
                out.print("\r\n");
                out.flush();
                // out.println();
            } else {
                out.print("\r\n");
                out.flush();
                // out.println();
            }
        } catch (Exception e) {
            e.printStackTrace();
            out.print(Response.HTTP_VERSION + " 500 Internal Server Error");
            out.print("\r\n");
            out.flush();
            // out.println();
        } finally {
            cleanup();
        }
    }

    private void cleanup() {
        log.debug("Initiating cleanup.");
        out.flush(); // Just to be safe and follow directions. out is already configured to
                     // auto-flush.

        if (fileOutSocket != null) {
            try {
                fileOutSocket.flush();
            } catch (IOException e) {
                /* Do nothing */ }
        }

        // Wait 250 millis = .25 seconds
        try {
            Thread.sleep(250);
        } catch (InterruptedException e) {
            /* Do nothing */ }

        try {
            socket.close();
        } catch (IOException e) {
            /* Do nothing */ }
        out.close();
        try {
            in.close();
        } catch (IOException e) {
            /* Do nothing */ }

        if (fileOutSocket != null) {
            try {
                fileOutSocket.close();
            } catch (IOException e) {
                /* Do nothing */ }
        }
        if (fileIn != null) {
            try {
                fileIn.close();
            } catch (IOException e) {
                /* Do nothing */ }
        }
    }
}