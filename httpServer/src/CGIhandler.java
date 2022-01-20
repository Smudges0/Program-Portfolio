import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;

public class CGIhandler {
	public static void invokeCGI(Request request, Response response) throws IOException {
		// // check request for content-length header
		// if ((request.getHeader("Content-Length") == null)
		// || !((request.getHeader("Content-Length")).matches("-?\\d+(\\.\\d+)?"))) { //
		// If getHeader for content length
		// // returns null OR if it doesnt
		// // match a numeric form, give error
		// response.setCode(411);
		// }

		// // check request for correct content type
		// if
		// (!(request.getHeader("Content-Type")).equals("application/x-www-form-urlencoded"))
		// { // if NOT urlencoded
		// response.setCode(500);
		// return;
		// }

		// // check URI for valid CGI script

		// // decode body into arguments
		// decode(request);

		// // build environment
		// StringBuilder env_contLen = new StringBuilder();
		// env_contLen.append("CONTENT_LENGTH=");
		// env_contLen.append(request.getHeader("CONTENT_LENGTH"));

		// pass to CGI via stdin
		Runtime cmd = Runtime.getRuntime(); // Get runtime object
		Process cgiScript = cmd.exec("." + request.getURI());

		BufferedWriter toCGI = new BufferedWriter(new OutputStreamWriter(cgiScript.getOutputStream()));
		BufferedReader fromCGI = new BufferedReader(new InputStreamReader(cgiScript.getInputStream()));

		toCGI.write(request.getBody());
		toCGI.flush();
		toCGI.close();

		String line;
		while ((line = fromCGI.readLine()) != null) {
			System.out.println(line);
		}

		fromCGI.close();
	}

	private static void decode(Request request) {
		StringBuilder decodedQuery = new StringBuilder();
		boolean prevEx = false;
		for (char c : request.getBody().toCharArray()) {
			if (c == '!') {
				if (prevEx) {
					prevEx = false;
					decodedQuery.append(c);
					continue;
				}
				prevEx = true;
				continue;
			}
			decodedQuery.append(c);
		}

		request.setBody(decodedQuery.toString());
		request.setHeader("Content-Length", Integer.toString(decodedQuery.length()));
	}

	public static void main(String[] args) throws IOException {
		Request basic = new Request();
		Request env = new Request();
		Request escaped = new Request();
		Response aResponse = new Response();

		basic.setURI("/cgi_bin/basic.cgi");
		basic.setBody("arg1=1&arg2=2");
		System.out.println("Invoke basic.cgi");
		CGIhandler.invokeCGI(basic, aResponse);

		env.setURI("/cgi_bin/env.cgi");
		env.setBody("arg1=1&arg2=2");
		System.out.println("Invoke env.cgi");
		CGIhandler.invokeCGI(env, aResponse);

		escaped.setURI("/cgi_bin/escaped.cgi");
		escaped.setBody("arg1=1&arg2=2");
		System.out.println("Invoke escaped.cgi");
		CGIhandler.invokeCGI(escaped, aResponse);
	}
}
