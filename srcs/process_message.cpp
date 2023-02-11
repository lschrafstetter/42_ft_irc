
Client Server::find_client(int fd) {
	std::map<int, Client>::iterator it;
    it = clients_.find(postbox[i].data.fd);
    return (*it).second;
}


void process_message_(std::vector<std::string> message, Client client) {
	//if there is no message then return error?

	//parsing into different commands
	if (message.at(1) == "PASS")
		authenticate_password(message);
	else
		std::cout <<"Command not recognized.\n";
	//what to do if no command matches?
}