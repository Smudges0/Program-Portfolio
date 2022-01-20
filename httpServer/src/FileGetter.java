import java.io.File;
import java.nio.file.FileSystems;
import java.nio.file.Files;
import java.text.ParseException;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;

public class FileGetter {
    private static final long MILLISECONDS_IN_YEAR = (long)1000 * 60 * 60 * 24 * 365;
    private static Map<String, String> mimeTypes = new HashMap<>(); 
    static {
        mimeTypes.put("htm", "text/html");
        mimeTypes.put("html", "text/html");
        mimeTypes.put("txt", "text/plain");
        mimeTypes.put("gif", "image/gif");
        mimeTypes.put("jpg", "image/jpeg");
        mimeTypes.put("jpeg", "image/jpeg");
        mimeTypes.put("png", "image/png");
        mimeTypes.put("bin", "application/octet-stream");
        mimeTypes.put("pdf", "application/pdf");
        mimeTypes.put("gz", "application/x-gzip");
        mimeTypes.put("zip", "application/zip");
    } 

    static void getFileInfo(Request request, Response response) {
        
        // Generating filename from docroot + URI, creating a File object
        StringBuilder filePathSB = new StringBuilder(PartialHTTP1Server.getDocRoot());
        filePathSB.append(request.getURI());
        String filePath = filePathSB.toString();

        File requestedFile = new File(filePath);
        if (!requestedFile.exists()) { // If file doesn't exist, 404 does not exist
            response.setCode(404);
            return;
        }

        if(requestedFile.isDirectory()) { // If file is a directory, 403 Forbidden
            response.setCode(403);
            return;
        }

        // if (!requestedFile.isRead()) { // If file is protected, 403 Forbidden
        //     response.setCode(403);
        //     return;
        // }

        boolean isReadable = Files.isReadable(FileSystems.getDefault().getPath(requestedFile.getAbsolutePath())); // Works for windows and linux (hopefully)
        if (!isReadable) { // If file is protected, 403 Forbidden
            response.setCode(403);
            return;
        }

        response.setRequestedFile(requestedFile);

        // Set Content length header
        response.setHeader("Content-Length", ((Long)requestedFile.length()).toString()); 

        // Set Content type header
        String[] parts = filePath.split("\\."); // Split filepath on . in order to read file extension
        if (parts.length <= 1) { // File has NO extension
            response.setHeader("Content-Type", "application/octet-stream");
        } else {
            String type = mimeTypes.get(parts[parts.length -1]); // Store mime type in String: type
            if (type == null) { // Unknown file extension, default to octet-stream
                response.setHeader("Content-Type", "application/octet-stream");
            } else { // Known extension, set mime type
                response.setHeader("Content-Type", type);
            }
        }
        
        // Set last modified header
        long lastModified = requestedFile.lastModified(); // last modified date of file
        long modifiedSince; // modified since date, from request
        String ifModifiedHeader = request.getHeader("If-Modified-Since"); // modified since date, from request: string format
        long expiresTime = new Date().getTime() + MILLISECONDS_IN_YEAR; // 1 year

        if (!request.getMethod().equals("HEAD")) { // If-modified-since does not apply to HEAD in http/1.0
            if (ifModifiedHeader != null) { // if "if modified since" header exists
                try {
                    modifiedSince = HttpUtils.getTimeStamp(ifModifiedHeader); // convert string format to Long date format
                } catch (ParseException e) { // Invalid date format, default to 0
                    modifiedSince = 0;
                }
                
                if (lastModified <= modifiedSince) { // compare lastModified and modifiedSince
                    response.setCode(304);
                    response.setHeader("Expires", HttpUtils.getHttpDate(expiresTime));
                    return;
                }
            }
        }
        
        response.setHeader("Last-Modified", HttpUtils.getHttpDate(lastModified)); // set last Modified header

        response.setHeader("Content-Encoding", "identity");

        // Expires should only be set for 200 or 304.  Not for other statuses.
        response.setHeader("Expires", HttpUtils.getHttpDate(expiresTime));

        // Allow should only be set for 200.  Not for other statuses.
        response.setHeader("Allow", "GET, POST, HEAD");

        // TODO: Clean up headers so no special handling in RequestHandler
    }
}
