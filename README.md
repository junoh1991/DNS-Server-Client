# Marcus Oh
# CSCI 4211
# Programming Assignment 1

Direction:
1. Type "make"
2. In any order, run the all five servers using ./server_app
    -local dns server takes the parameters:
         ID
         PORT
    -root dns server takes the parameters:
         ID
         PORT
         null
         server.dat
    -each tld dns server takes the parameters:
         ID
         PORT
         *.dat
         null
          

3. Run the client using ./client_app ID IP PORT

4. Everything should be running now. Use client_app to request hostnames in the format <ID, hostname, I/R>

5. Servers can be closed using ctrl-c.
   Client can be closed by typing q




Assumptions:
    - The programs were designed to closely mimic to produce the almost exact same output.
    - DNS server have 3rd parameter to be 'null' and tld server have 4rd parameter to be 'null'.
      This was required so the program identifies each server by their parameters.
    - Each request does not exceed 100 chracters.
    - Each entry in the dat file does not exceed 100 characters.
    - Note that ports for servers are predefined in the assignment pdf. Therefore, IP/Ports are hardcoded into
      the server program. Changing server.dat must be followed with correct port numbers when initializing servers.
