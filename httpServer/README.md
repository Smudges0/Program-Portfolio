## To run or debug locally in vscode, with local /doc_root directory

- Make sure doc_root directory exists in the project.  If not, do this:
    - tar -zxvf doc_root.tar.gz
    - Copy the doc_root folder into the project at the same level as src.

- Change the PartialHTTP1Server.docRoot setting from "." to "./doc_root".
- Run or debug the Server launcher task. (Port 3456)

## To test with HTTPClient

- Run the Client launcher task.
- Enter the request.  No time limit.  The socket is not opened yet.
- Hit enter to send to server.
- You can enter another request afterwards.

## To test with curl

- Open a bash terminal
- Enter curl command:
- curl --http1.0 -i http://localhost:3456/index.html

## To test with Firefox

- type "about:config" in the location bar.  Press "Accept the Risk and Continue"
- type "network.http.status".  Edit value from 1.1. to 1.0.  (Remember to switch it back when you are done testing.)
- Press F12 to open debugger window.  View Network tab.
- Go to http://localhost:3456/index.html
- Select request for index.html.  View Headers.  Toggle "Raw" switch for Request and Response to view.
- 304 response does not display correctly in Firefix, but it's OK.
- Chrome does not support HTTP/1.0 so can't test with it.

## To test with HTTPServerTester in Linux (Official way)

Download and install the doc_root anywhere:
- tar -zxvf doc_root.tar.gz
- cd doc_root

Export the httpserver java project in vscode as a jar and extract class files to doc_root and start server:
- cp /mnt/c/Users/Simon/Documents/github/cs352/httpserver/httpserver.jar .
- jar -xvf httpserver.jar
- java -cp . PartialHTTP1Server 3456

Download and run the HTTPServerTester.jar in another bash terminal (from any location)
- java -jar HTTPServerTester.jar localhost 3456

All tests should pass.

## To test with HTTPServerTester while debugging PartialHTTP1Server

- Follow instructions above to debug server in vscode.
- In a bash terminal, run:
- java -jar HTTPServerTester.jar localhost 3456

## To debug HTTPServerTester

- There is a version of httpserver project in C:\Users\Simon\Documents\github\TestingHttpServerBranch\httpserver which has a HTTPServerTester.java with debugging.  You need to find the bin directory of the project by right-clicking the httpserver line in JAVA PROJECTS view, and select "Reveal in Explorer".
Then go to the httpserver_* directory and into the bin directory.  Add a new folder called "resources" and copy the TestCases.txt file to it.  Then you can run/debug the HTTPServerTester using this launcher entry:

        {
            "type": "java",
            "name": "HTTPServerTester",
            "request": "launch",
            "mainClass": "HTTPServerTester",
            "args": " localhost 3456",
            "console": "externalTerminal"
        },

- Test against either the server started using the command line, or via vscode.

## To create a tar file with all your classes

- In the doc_root directory where you unjarred your httpserver.jar file, run this:
- tar -zcvf PartialHTTP1Server.tar.gz *.class

## SCP/SSH to transfer files to iLabs

- scp doc_root.tar.gz ssb170@crayon.cs.rutgers.edu:~
- ssh ssb170@crayon.cs.rutgers.edu