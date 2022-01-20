import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

public class Request {
    // Methods we need to handle (either accept, or return "unsupported" code)
    private static final String GET = "GET";
    private static final String POST = "POST";
    private static final String HEAD = "HEAD";
    private static final String PUT = "PUT";
    private static final String LINK = "LINK";
    private static final String UNLINK = "UNLINK";
    private static final String DELETE = "DELETE";

    private static final Set<String> validMethods = new HashSet<>();
    private static final Set<String> supportedMethods = new HashSet<>();

    private String method;
    private String URI;
    private String httpVersion;
    private String body;
    private Map<String, String> headers = new HashMap<>();

    static {
        validMethods.addAll(Arrays.asList(GET, POST, HEAD, PUT, LINK, UNLINK, DELETE));
        supportedMethods.addAll(Arrays.asList(GET, POST, HEAD));
    }

    public static boolean isMethodValid(String aMethod) {
        return validMethods.contains(aMethod);
    }

    public static boolean isMethodSupported(String aMethod) {
        return supportedMethods.contains(aMethod);
    }

    public String getMethod() {
        return method;
    }

    public void setMethod(String method) {
        this.method = method;
    }

    public String getURI() {
        return URI;
    }

    public void setURI(String uRI) {
        URI = uRI;
    }

    public String getHttpVersion() {
        return httpVersion;
    }

    public void setHttpVersion(String httpVersion) {
        this.httpVersion = httpVersion;
    }

    public String getHeader(String key) {
        if (headers.containsKey(key)) {
            return headers.get(key);
        }

        return null;
    }

    public void setHeader(String key, String value) {
        headers.put(key, value);
    }

    public String getBody() {
        return body;
    }

    public void setBody(String body) {
        this.body = body;
    }
}
