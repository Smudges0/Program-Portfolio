import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
 
public class Logger {
    private String className;
    private StringBuilder sb = new StringBuilder();
    private DateFormat dateFormat = new SimpleDateFormat("MM/dd/yyyy hh:mm:ss.SSS a z");
 
    // Call this class like this: 
    // Logger log = Logger.getLogger(HttpServer.class);
    // log.debug(“Testing 123…”);
    public static Logger getLogger(final Class<?> clazz) {
        return new Logger(clazz);
    }
 
    private Logger(final Class<?> clazz) {
        this.className = clazz.getSimpleName();
    }
 
    private synchronized void log(String level, String msg) {
        sb.setLength(0);
        String timeStr = dateFormat.format(new Date());
        sb.append(timeStr).append(" ").append(level).append(" [").append(className).append(":").append(Thread.currentThread().getName()).append("] ").append(msg);
        System.out.println(sb);
    }
 
    public synchronized void debug(String msg) {
        log("DEBUG", msg);
    }
 
    public synchronized void info(String msg) {
        log("INFO", msg);
    }
 
    public synchronized void error(String msg) {
        log("ERROR", msg);
    }
}
