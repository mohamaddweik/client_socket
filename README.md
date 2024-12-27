HTTP Client Application 

Authored by Mohamad Dweik

==Description==
This project implements a simple HTTP client in C that can parse URLs, handle HTTP GET requests, and manage HTTP redirections. The program allows users to specify query parameters and follows HTTP redirection headers when encountered.

==Program Database==
1)URL Parsing and Validation:

  Parses the input URL into host, path, and port.
  Validates the URL format and port numbers.

2)HTTP Request Handling:

  Constructs an HTTP GET request string based on the given URL and parameters.

3)Socket Communication:

  Establishes a connection to the server using sockets.
  Sends the HTTP request and reads the server response.
4)Redirection Management:

  Detects 3XX status codes and extracts the Location header for redirection.

==Functions==
Main Functions
1)print_usage: Displays usage instructions for the program.
2)validate_port: Ensures the port number is valid.
3)parse_url: Parses the input URL into host, path, and port components.
4)parse_arguments: Parses and validates command-line arguments.
5)build_http_request: Constructs the HTTP GET request string, including query parameters.
6)connect_to_server: Establishes a socket connection to the server.
7)send_and_receive: Sends the HTTP request, receives the response, and checks for redirections.

Helper Functions
1)gethostbyname: Resolves the hostname to an IP address.
2)strncpy/strchr: Used to manipulate strings and extract URL components.

==Program Files==
client.c: Contains all the program logic, including argument parsing, URL handling, socket communication, and response handling.

==How to Compile==
gcc -Wall –o client client.c

==Input==
The program accepts the following command-line arguments:

./http_client [-r n <param1=value1 param2=value2 ...>] <URL>

-r n: Specifies n query parameters in the form param=value.
<URL>: The HTTP URL to send the GET request to (must start with http://).

==Output==
1)Displays the constructed HTTP GET request.
2)Prints the server's HTTP response to the console.
3)Handles HTTP redirections (3XX status codes) and re-sends requests to the redirected location until a final response is received.
