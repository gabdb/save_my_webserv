# Documentation Gab
	- Class TCPserver()
		### member Variables/structures
		  collapsed:: true
			- ####  Listeners (serveur)
				- ```cpp
				  struct Listener {
				    int fd;
				    std::string host;
				    int port;
				    std::vector<const Block *> servers; // pointers to respective server blocks who are using this listener
				  };
				  
				  vector<Listener> _listeners;
				  ```
				- vecteur qui contiendra chaque entrée du serveur (host:port uniques)
				- La struct Listener stock son fd, host, port et chaque block serveur qu'il doit ecouter (servira a identifier les valeurs lors du parsing des requetes d'un client)
			- #### Client (navigateur)
			  collapsed:: true
				- ```cpp
				  struct Client {
				    int fd;
				    std::string recvBuffer;   // raw bytes received but not yet processed
				    std::string sendBuffer;   // bytes waiting to be sent
				    bool wantRead;            // waiting on response signal
				    bool wantWrite;           // waiting on request signal
				    const Block *serverBlock; // which server this TCP connexion is attached to
				  };
				  
				  
				  std::map<int, Client> _clients
				  ```
				- ici on crée un map <int, Client> ou chaque Client est map a son int respectif.
				  J'explique plus tard dans run() pq j'ai fait ca.
				- Sert a stocker les valeurs recues et a envoyer d'un client (avec des bools servant a dire a poll "requetes en attente/reponse prete")
		- ### Fonctions
			- #### init() (InitServer.cpp)
			  collapsed:: true
				- ##### initListeners()
				  collapsed:: true
					- Utilité: Initialize chaque entrée du serveur. s'assure que chaque entrée est une combinaison de host:port unique
					- 1) On initialize une map reliant chaque host:port unique a une struct Linstener
					  collapsed:: true
						- ```cpp
						    std::map<std::string, Listener> listenerMap;
						  ```
					- 2) loop a travers chaque block serveur
					  collapsed:: true
						- ```cpp
						  //ligne 75
						  for (size_t i = 0; i < servers.size(); ++i) {
						      const Block &srvBlock = servers[i];
						  
						  ```
					- 3) On extrait les host et ports du block serveur actuel, creation d'une "key" pour chaque combinaison existant dans ce block (host:port)
					  collapsed:: true
						-
						- ```cpp
						      std::vector<std::string> hosts = getStringValues(srvBlock, "host");
						      // TODO: enhance error handling logic within getStringValues
						  
						      std::vector<int> ports = getIntValues(srvBlock, "listen");
						  
						      // Check if "host:port" is allready in map. If yes, append to key's server list. Otherways, create mapping
						      for (size_t h = 0; h < hosts.size(); ++h) {
						        for (size_t p = 0; p < ports.size(); ++p) {
						          const int port = ports[p];
						  
						          std::string key = hosts[h] + ":" + intToStr(port);
						  
						  ```
					- 4) On compare la "key" a notre list (std::map<std::string>)
					  collapsed:: true
						- ```cpp
						          std::map<std::string, Listener>::iterator it = listenerMap.find(key);
						          if (it == listenerMap.end()) {
						  ```
					- 5) Si "key" n'existe pas dans notre map actuelle, on crée un Listener pour cette combinaison port:host et ajoute le block serveur actuel a la liste de serveurs du Listener (std::vector<const Block *> servers dans struct Listener)
					  collapsed:: true
						- ```cpp
						          if (it == listenerMap.end()) {
						            Listener lst;
						            lst.fd = -1;
						            lst.host = hosts[h];
						            lst.port = port;
						            lst.servers.push_back(&srvBlock);
						  
						            listenerMap.insert(std::make_pair(key, lst));
						          }
						  ```
					- 6) Si la "key" corerspond a une valeur std::string de notre map, on ajoute le serveur actuel a la liste de serveurs du listener
					  collapsed:: true
						- ```cpp
						          else {
						            it->second.servers.push_back(&srvBlock);
						  ```
				- ##### createListeningSockets()
				  collapsed:: true
					- 1) On loop a travers chaque listener crée dans la fonction precedante
						- ```cpp
						    for (size_t i = 0; i < _listeners.size(); ++i) {
						  
						      Listener &lst = _listeners[i];
						  ```
					- 2) Pour chaque listener, on initialize un socket qu'on attache a un fd, on bind et on listen
						- ```cpp
						  int fd = socket(AF_INET, SOCK_STREAM, 0);
						  if (bind(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0)
						  if (listen(fd, SOMAXCONN) < 0)
						  ```
					- 3) On set le socket en mode non bloquant
						- ```cpp
						  if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
						  ```
					- 4) on attache le fd (socket) a notre Listener
						- ```cpp
						  lst.fd = fd;
						  ```
			- #### run() (RunServer.cpp)
				- Utilité: system principal du serveur servant a gerer les connexions clients ainsi que leurs requetes et les reponses respectives
				- loop "while(true)", tourne tt le temps
				- 1) on initialize la structure pollfd, structure utilisée par poll pour gerer les differentes sockets attachées au serveur
					- 1.1) on commence par attacher chaque port d'entrée du serveur a notre struct pollfd. Note: les fd de listeners sont statiques, 1 fd d'entrée de serveur est crée lors du lancement du serveur et ne sera supprimé que lorsque celui ci est arreté => ne changeront jamais en cours d'utilisation
					  collapsed:: true
						- ```cpp
						  void TCPserver::init_pollfds(std::vector<struct pollfd> &pfds) {
						    // add _listener fds to poll's fds to read from (POLLIN)
						    pfds.reserve(_listeners.size() + _clients.size());
						    for (size_t i = 0; i < _listeners.size(); ++i) {
						      struct pollfd p;
						      p.fd = _listeners[i].fd;
						      p.events = POLLIN;
						      p.revents = 0;
						      pfds.push_back(p);
						    }
						  ```
					- 1.2) Ensuite viennent les connexions client. Ceux ci sont dynamiques, la liste de clients connectés au serveur changeront au fil du temps (fermeture du nav, ouverture d'un autre nav, nouvel utilisateur, ...), raison pour laquelle on a crée un std::map<int, Client> _clients et non un std::vector<Client> comme pour _listeners. 
					  Je m'explique: Si nous avions crée un "std::vector<Client> _clients" , nous aurions donc crée une loop (for int i = 0; i < _clients.size(); ++i) afin de naviguer notre liste de clients connectés au serveur, se referant a un index pour se situer et naviguer parmi les clients de la liste. 
					  Cela etant dit: si plus tard dans l'execution (ReadfromClient) !!!durant la meme iteration de la loop while(true)!!! on ferme une connexion client, notre vecteur reindexerai les clients restants (client[i + 1] => client[i] et ainsi de suite).
					  Or, l'iteration incrementerai tout de meme de 1 pour passer au client suivant. Resultat: il y a une desynchronisation de tous les clients restants et on ne process pas 1 client (meilleur des cas) 
					  DONC on crée une std::map pour rechercher les clients par fd, et non par index dans un vecteur
						- ```cpp
						  void TCPserver::init_pollfds(std::vector<structpollfd> &pfds) {
						    // ...
						    // code 1.1 
						    // ...
						    
						    for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
						      struct pollfd p;
						      p.fd = it->first;
						      p.events = 0;
						      if (it->second.wantRead)
						        p.events |= POLLIN; // |= is "OR" bitwise op.
						      if (it->second.wantWrite && !it->second.sendBuffer.empty())
						        p.events |= POLLOUT;
						      p.revents = 0;
						      pfds.push_back(p);
						    }
						  }
						  
						  ```
						-
				- 2) On lance ensuite poll sur la liste de struct pollfd
				  collapsed:: true
					- ```cpp
					      int ready = poll(&pfds[0], pfds.size(), -1);
					  ```
				- 3) on crée notre loop pour que chaque entrée serveur puisse accepter des nouvelles connexions clients en mode non-bloquant (le 3 way handshake du protocol est géré au niveau du kernel, pas notre taf)
				  collapsed:: true
					- ```cpp
					      for (size_t i = 0; i < _listeners.size(); ++i) {
					        if (pfds[i].revents & POLLIN)
					          acceptNewClient(pfds[i].fd);
					  
					  void TCPserver::acceptNewClient(int listenFd) {
					    while (true) {
					      int clientFd = accept(listenFd, NULL, NULL);
					      if (clientFd < 0)
					        break;
					      if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0) {
					        ::close(clientFd);
					        continue;
					      }
					  
					      Client c;
					      c.fd = clientFd;
					      c.recvBuffer.clear();
					      c.sendBuffer.clear();
					      c.wantRead = true;
					      c.wantWrite = false;
					      c.serverBlock = NULL; // TODO: find which Listener (server) this belongs to
					  
					      _clients[clientFd] = c;
					    }
					  }
					  
					  ```
					-
				- 4) Pour chaque client existant dans notre std::map, on verifie si il y a une action a executer
				  collapsed:: true
					- ```cpp
					      for (size_t i = _listeners.size(); i < pfds.size(); ++i) {
					        int fd = pfds[i].fd;
					        std::map<int, Client>::iterator it = _clients.find(fd);
					        if (it == _clients.end())
					          continue; // if pfds[i].fd not found in clients, jump to next iteration of i
					  
					        // Check for end of client life, if EOL: clean up
					        Client &client = it->second;
					        if (pfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
					          closeClientConnexion(fd);
					          continue;
					        }
					        if (pfds[i].revents & POLLIN)
					          ReadfromClient(client); // Gab TODO: Start implementing recv logic here
					        if (pfds[i].revents & POLLOUT)
					          WritetoClient(client); // Gab TODO: Start implementing send logic here
					      }
					  
					  ```
				-
				- Note: run() est une loop continue. ca veut dire que les fonctions a l'interieur s'execute de maniere sequentielle et qu'une nouvelle liste de struct pollfd est crée a chaque iteration de la loop, permetant de dynamiquement modifier la liste de clients (nouvelles connexions/connexions expirées/...).
				-
			-
