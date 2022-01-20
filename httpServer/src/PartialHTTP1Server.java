
import java.io.IOException;
import java.io.PrintWriter;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.RejectedExecutionHandler;
import java.util.concurrent.SynchronousQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

/**
 * Author: Simon Bruce 
 * Date: 9/28/2020
 * Class: CS352 section 5
 * Description: Assignment 1 simple http 1.0 multithreaded server, only supports GET, POST, HEAD (GET and POST are identical)
 */
public class PartialHTTP1Server {
    private static String docRoot = ".";
    private ExecutorService executorService;
    private SynchronousQueue<Runnable> requestQueue;

    private Logger log;

    private int minPoolSize = 5;
    private int maxPoolSize = 50;
    private long keepAliveTimeSeconds = 30;
    private int portNum;

    private Socket requestSocket;

    PartialHTTP1Server(int portNum) {
        this.portNum = portNum;
        log = Logger.getLogger(this.getClass());
    }

    private void start() {
        requestQueue = new SynchronousQueue<Runnable>();

        RejectedExecutionHandler rejectHandler = new RejectedExecutionHandler() {
            @Override
		    public void rejectedExecution(Runnable task, ThreadPoolExecutor executor) {
                try {
                    log.info("Unable to create more threads. Rejecting request.");
                    PrintWriter out = new PrintWriter(requestSocket.getOutputStream(), true);
                    out.print(Response.HTTP_VERSION +" 503 Service Unavailable");
                    out.print("\r\n");
                    out.flush();
                    //out.println();
                    requestSocket.close();
                }
                catch (IOException e) {
                    log.error("Unable to close socket");
                }
		    }
        };

        executorService = new ThreadPoolExecutor(minPoolSize, maxPoolSize, keepAliveTimeSeconds, TimeUnit.SECONDS, requestQueue, rejectHandler);

        try (ServerSocket serverSocket = new ServerSocket(portNum)) { 
            while (true) {
                log.info("Waiting for client connection. Queue size = "+((ThreadPoolExecutor)executorService).getQueue().size()+". "+"Active pool threads = "+((ThreadPoolExecutor)executorService).getActiveCount());
                requestSocket = serverSocket.accept();
                log.info("Accepted client connection. Socket port "+requestSocket.getPort());
                try {
                    executorService.execute(new RequestHandler(requestSocket));

                } catch (Exception e) {
                    e.printStackTrace();
                    // Keep going.
                }
            }
        } catch (IOException e) {
            log.error("I/O exception.  Could not listen on port");
            System.exit(-1);
            // NOTE: On Win10 and iLabs linux, starting a second server on the same port will generate IOException.
            // But on WSL Ubuntu, it looks like you can start 2 listener processes on the same port.  Only one
            // gets requests though.
        }
    }
    
    public static void main(String[] args) throws IOException {
        Logger log = Logger.getLogger(PartialHTTP1Server.class);
        if (args.length != 1) {
            log.error("Usage: java PartialHTTP1Server <port number>");
            System.exit(1);
        }

        int portNumber = Integer.parseInt(args[0]);
        PartialHTTP1Server server = new PartialHTTP1Server(portNumber);
        server.start();
    }

    public static String getDocRoot() {
        return docRoot;
    }
}