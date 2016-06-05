

#ifndef GEODB_H_
#define GEODB_H_

#include <string>
#include <set>
#include "sqlite3.h"
#include "dfa.h"

using namespace std;

class geodb
{
	private:

		sqlite3* db;

		string path_db;

	public:

		geodb(string path_of_db);

		void 	connect();

		void 	close();

		void 	create_table();

		void 	insert();

		void 	get_data();

		set<string>* 		get_prefixes_for_user(string user, int length);

		vector<string>* 	get_minitraj_for_prefix_and_user(string user, string prefix);

		vector<string>* 	get_minitraj_for_prefix_and_allUsers_EXCEPT_user(string user, string prefix);

		// Usato nella similarità tra utenti, per ricavare le stringhe negative comuni a due utenti
		vector<string>* get_minitraj_for_prefix_and_allUsers_EXCEPT_2_users(string user1, string user2, string prefix);

		void	get_num_minitraj_for_user(string user, int  prefix_lenght);

};



#endif
