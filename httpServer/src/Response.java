import java.io.File;
import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;

public class Response {
    public static final String HTTP_VERSION = "HTTP/1.0";
    private static Map<Integer, String> codeMessage = new HashMap<>();

    private int code;
    private Map<String, String> headers = new HashMap<>();
    private File requestedFile;

    static {
        codeMessage.put(200, "OK");
        codeMessage.put(304, "Not Modified");
        codeMessage.put(400, "Bad Request");
        codeMessage.put(403, "Forbidden");
        codeMessage.put(404, "Not Found");
        codeMessage.put(408, "Request Timeout");
        codeMessage.put(411, "Length Required");
        codeMessage.put(500, "Internal Server Error");
        codeMessage.put(501, "Not Implemented");
        codeMessage.put(503, "Service Unavailable");
        codeMessage.put(505, "HTTP Version Not Supported");
    }

    public static String getCodeMessage(int code) {
        return codeMessage.get(code);
    }

    public int getCode() {
        return code;
    }

    public void setCode(int code) {
        this.code = code;
    }

    public String getHeader(String key) {
        return headers.get(key);
    }

    public void setHeader(String key, String value) {
        headers.put(key, value);
    }

    public boolean isReadyToSend() {
        return (code != 0);
    }

    public Set<Entry<String, String>> getHeaderEntries() {
        return headers.entrySet();
    }

    public File getRequestedFile() {
        return requestedFile;
    }

    public void setRequestedFile(File requestedFile) {
        this.requestedFile = requestedFile;
    }
}
