/*
 * geodb.cpp
 *
 *      Author: Gabriele
 */


#include <stdio.h>
#include <iostream>
#include <string>
#include <set>
#include "sqlite3.h"
#include "utilities.h"
#include <boost/lexical_cast.hpp>

#include "geodb.h"

using namespace std;

static int list_of_prefixes_for_user(void *data, int n_column, char **values, char **azColName);


geodb::geodb(string path_of_db)
{
	path_db = path_of_db;
}

void geodb::connect()
{
	//char* zErrMsg = NULL;
	int rc;


	// Open tha connection to database, if doesn't exists create it
	rc = sqlite3_open(path_db.c_str(), &db);


	if( rc ){
	  fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
	  //exit(0);
	/*}else{
	  fprintf(stderr, "Opened database successfully\n");
	  */
	}
}

void geodb::close()
{
	sqlite3_close(db);
}

//void*,    /* Data provided in the 4th argument of sqlite3_exec() */
//int,      /* The number of columns in row */
//char**,   /* An array of strings representing fields in the row */
//char**    /* An array of strings representing column names */
static int callback(void *data, int n_column, char **values, char **azColName){
   int i;

   //fprintf(stderr, "%s: ", (const char*)data);

   for(i=0; i<n_column; i++)
   {
	   printf("%s = %s\n", azColName[i], values[i] ? values[i] : "NULL");
   }

   printf("\n");

   return 0;
}


void geodb::create_table()
{

	char *zErrMsg = 0;
	int  rc;
	char *sql;


	this->connect();


	/* Create SQL statement */
	sql = "CREATE TABLE COMPANY("  \
		 "ID INT PRIMARY KEY     NOT NULL," \
		 "NAME           TEXT    NOT NULL," \
		 "AGE            INT     NOT NULL," \
		 "ADDRESS        CHAR(50)," \
		 "SALARY         REAL );";


	/* Execute SQL statement */
	rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);


	if( rc != SQLITE_OK ){
		fprintf(stderr, "SQL -create table- error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	/*}else{
		fprintf(stdout, "Table created successfully\n");*/
	}

	sqlite3_close(db);

}



void geodb::insert()
{
	char *zErrMsg = 0;
	int rc;
	char *sql;

	this->connect();


	/* Create SQL statement */
	sql = "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) "  \
		 "VALUES (1, 'Paul', 32, 'California', 20000.00 ); " \
		 "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) "  \
		 "VALUES (2, 'Allen', 25, 'Texas', 15000.00 ); "     \
		 "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)" \
		 "VALUES (3, 'Teddy', 23, 'Norway', 20000.00 );" \
		 "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)" \
		 "VALUES (4, 'Mark', 25, 'Rich-Mond ', 65000.00 );";


	/* Execute SQL statement */
	rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);


	if( rc != SQLITE_OK ){
	  fprintf(stderr, "SQL -insert- error: %s\n", zErrMsg);
	  sqlite3_free(zErrMsg);
	/*}else{
	  fprintf(stdout, "Records created successfully\n");*/
	}

	sqlite3_close(db);
}


void geodb::get_data()
{
	char *zErrMsg = 0;
	int rc;
	char *sql;
	const char* data = "Callback function called";

	this->connect();

	/* Create SQL statement */
	//sql = "SELECT * from COMPANY";
	sql = "SELECT * from gprefix5";



	/* Execute SQL statement */
	rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);


	if( rc != SQLITE_OK ){
	  fprintf(stderr, "SQL -select- error: %s\n", zErrMsg);
	  sqlite3_free(zErrMsg);
	/*}else{
	  fprintf(stdout, "Operation done successfully\n");*/
	}


	sqlite3_close(db);
}



//void*,    /* Data provided in the 4th argument of sqlite3_exec() */
//int,      /* The number of columns in row */
//char**,   /* An array of strings representing fields in the row */
//char**    /* An array of strings representing column names */
static int list_of_prefixes_for_user(void *data, int n_column, char **values, char **azColName)
{

	// Need cast to be used
   set<string>* p = (set<string>*) data;


   // Insert all the prefixes with at least one minitrajector
   // NB: "set" container can have only unique elements, also if many equals elements are insert
   //for(int i=0; i<n_column; i++){
   p->insert(values[0]);

	   //printf("%s = %s\n", azColName[i], values[i] ? values[i] : "NULL");
   //}

   return 0;
}


// Return a pointer to a set<string>, it's the set of all prefixes for user with that length
set<string>* geodb::get_prefixes_for_user(string user, int length)
{
	char *zErrMsg = 0;
	int rc;
	const char *sql;


	//vector with all prefix for user
	set<string>* prefixes = new set<string>;


	// Connect DB
	this->connect();


	/* Create SQL statement */
	string l = intTostring(length);
	string sql_query ="SELECT prefix FROM gprefix"+l+" WHERE id_user = '"+user+"'";
	// Puoi pensare di fare una query che restituisce gi� gli elementi univoci
	// velocizzando gli inserimenti


	sql = sql_query.c_str();


	/* Execute SQL statement */
	rc = sqlite3_exec(db, sql, list_of_prefixes_for_user, (void*)prefixes, &zErrMsg);


	// Print elements inside set, they are prefixes for the user
	/*set<string>::iterator it;

	cout << "Prefixes of length "+l+" for user "+user+":"<<endl;
	for (it=prefixes->begin(); it!=prefixes->end(); ++it)
	   cout << ' ' << *it;
	cout << '\n';*/


	// If ERROR
	if( rc != SQLITE_OK ){
	  fprintf(stderr, "SQL -select- error: %s\n", zErrMsg);
	  sqlite3_free(zErrMsg);
	}

	return prefixes;
}





//static int list_of_minitraj_for_prefix_and_user(void *data, int n_column, char **values, char **azColName)
//{
//
//   // Need cast to be used
//   set<string>* p = (set<string>*) data;
//
//
//   // Insert all the minitraj for the prefix and the selected user
//   // NB: "set" container can have only unique elements, also if many equals elements are insert
//   for(int i=0; i<n_column; i++)
//	   p->insert(values[i]);
//
//
//
//   return 0;
//}



static int list_of_minitraj_for_prefix_and_user(void *data, int n_column, char **values, char **azColName)
{

   // Need cast to be used
	vector<string>* p = (vector<string>*) data;


   // Insert all the minitraj for the prefix and the selected user
   // NB: "set" container can have only unique elements, also if many equals elements are insert
   for(int i=0; i<n_column; i++)
	   p->push_back( values[i] );




   return 0;
}


vector<string>* geodb::get_minitraj_for_prefix_and_user(string user, string prefix)
{
	char *zErrMsg = 0;
	int rc;
	const char *sql;


	//vector with all prefix for user
	vector<string>* minitraj = new vector<string>;


	// Connect DB
	//this->connect();


	/* Create SQL statement */
	string l = intTostring(prefix.length());
	string sql_query ="SELECT minitraj FROM gprefix"+l+" WHERE id_user = '"+user+"' AND prefix='"+prefix+"'";		// NB: Take also duplicate!
	sql = sql_query.c_str();


	/* Execute SQL statement */
	rc = sqlite3_exec(db, sql, list_of_minitraj_for_prefix_and_user, (void*)minitraj, &zErrMsg);


	// Print elements inside set, they are prefixes for the user
	/*set<string>::iterator it;

	cout << "Minitraj. for "+prefix+" are:" << endl;
	for (it=minitraj->begin(); it!=minitraj->end(); ++it)
	   cout << ' ' << *it << endl << endl;
	cout << '\n';*/


	// If ERROR
	if( rc != SQLITE_OK ){
	  fprintf(stderr, "SQL -select- error: %s\n", zErrMsg);
	  sqlite3_free(zErrMsg);
	}


	return minitraj;

}


vector<string>* geodb::get_minitraj_for_prefix_and_allUsers_EXCEPT_user(string user, string prefix)
{
	char *zErrMsg = 0;
	int rc;
	const char *sql;


	//vector with all prefix for user
	vector<string>* minitraj = new vector<string>;


	// Connect DB
	//this->connect();


	/* Create SQL statement */
	string l = intTostring(prefix.length());
	//string sql_query ="SELECT minitraj FROM gprefix"+l+" WHERE id_user != '"+user+"'";
	string sql_query ="SELECT minitraj FROM gprefix"+l+" WHERE prefix = '"+prefix+"' AND minitraj NOT IN " \
					+"(SELECT minitraj FROM gprefix"+l+" WHERE id_user = '"+user+"')";													 // NB: Take also duplicate!
	sql = sql_query.c_str();

	clock_t tStart = clock();

	/* Execute SQL statement */
	rc = sqlite3_exec(db, sql, list_of_minitraj_for_prefix_and_user, (void*)minitraj, &zErrMsg);

	//printf("Second to insert from DB to set: %.2fs\n",(double)(clock() - tStart)/CLOCKS_PER_SEC);


	// Print elements inside set, they are prefixes for the user
	/*set<string>::iterator it;

	cout << "Minitraj. for "+prefix+" are:" << endl;
	for (it=minitraj->begin(); it!=minitraj->end(); ++it)
	   cout << ' ' << *it << endl << endl;
	cout << '\n';*/


	// If ERROR
	if( rc != SQLITE_OK ){
	  fprintf(stderr, "SQL -select- error: %s\n", zErrMsg);
	  sqlite3_free(zErrMsg);
	}


	return minitraj;
}



vector<string>* geodb::get_minitraj_for_prefix_and_allUsers_EXCEPT_2_users(string user1, string user2, string prefix)
{
	char *zErrMsg = 0;
	int rc;
	const char *sql;


	//vector with all prefix for user
	vector<string>* minitraj = new vector<string>;


	// Connect DB
	//this->connect();


	/* Create SQL statement */
	string l = intTostring(prefix.length());
	//string sql_query ="SELECT minitraj FROM gprefix"+l+" WHERE id_user != '"+user+"'";
	string sql_query ="SELECT minitraj FROM gprefix"+l+" WHERE prefix = '"+prefix+"' AND minitraj NOT IN " \
					+"(SELECT minitraj FROM gprefix"+l+" WHERE id_user = '"+user1+"' OR id_user = '"+user2+"')";													 // NB: Take also duplicate!
	sql = sql_query.c_str();

	clock_t tStart = clock();

	/* Execute SQL statement */
	rc = sqlite3_exec(db, sql, list_of_minitraj_for_prefix_and_user, (void*)minitraj, &zErrMsg);

	//printf("Second to insert from DB to set: %.2fs\n",(double)(clock() - tStart)/CLOCKS_PER_SEC);


	// Print elements inside set, they are prefixes for the user
	/*set<string>::iterator it;

	cout << "Minitraj. for "+prefix+" are:" << endl;
	for (it=minitraj->begin(); it!=minitraj->end(); ++it)
	   cout << ' ' << *it << endl << endl;
	cout << '\n';*/


	// If ERROR
	if( rc != SQLITE_OK ){
	  fprintf(stderr, "SQL -select- error: %s\n", zErrMsg);
	  sqlite3_free(zErrMsg);
	}


	return minitraj;
}



// SCRIVI LA FUNZIONE CHE TIRA FUORI TUTTI I PERCORSI NON ANCORA SCOMPOSTI DELL'UTENTE
//vector<string>* geodb::get_paths_of_user(string user)
//{
//	char *zErrMsg = 0;
//	int rc;
//	const char *sql;
//
//
//	//vector with all prefix for user
//	vector<string>* paths = new vector<string>;
//
//
//	// Connect DB
//	//this->connect();
//
//
//	/* Create SQL statement */
//	string sql_query ="SELECT minitraj FROM gprefix"+l+" WHERE id_user = '"+user+"';		// NB: Take also duplicate!
//	sql = sql_query.c_str();
//
//
//	/* Execute SQL statement */
//	rc = sqlite3_exec(db, sql, list_of_minitraj_for_prefix_and_user, (void*)paths, &zErrMsg);
//
//
//	// Print elements inside set, they are prefixes for the user
//	/*set<string>::iterator it;
//
//	cout << "Minitraj. for "+prefix+" are:" << endl;
//	for (it=minitraj->begin(); it!=minitraj->end(); ++it)
//	   cout << ' ' << *it << endl << endl;
//	cout << '\n';*/
//
//
//	// If ERROR
//	if( rc != SQLITE_OK ){
//	  fprintf(stderr, "SQL -select- error: %s\n", zErrMsg);
//	  sqlite3_free(zErrMsg);
//	}
//
//
//	return minitraj;
//
//}

















static int number_minitraj_for_user(void *data, int n_column, char **values, char **azColName)
{

   // Need cast to be used
	int* n = (int*) data;


   // Insert all the minitraj for the prefix and the selected usermake
   *n = boost::lexical_cast<int>(values[0]);

   return 0;
}


void geodb::get_num_minitraj_for_user(string user, int  prefix_lenght)
{
	char *zErrMsg = 0;
	int rc;
	const char *sql;


	//vector with all prefix for user
	int* n_mini_traj = new int();
	*n_mini_traj = -1;


	// Connect DB
	//this->connect();


	/* Create SQL statement */
	string sql_query ="SELECT count(distinct minitraj) FROM gprefix"+intTostring(prefix_lenght)+" WHERE id_user = '"+user+"'";		// NB: Take also duplicate!
	sql = sql_query.c_str();


	/* Execute SQL statement */
	rc = sqlite3_exec(db, sql, number_minitraj_for_user, (void*)n_mini_traj, &zErrMsg);


	cout << user << " " << (*n_mini_traj) << endl;


	// If ERROR
	if( rc != SQLITE_OK ){
	  fprintf(stderr, "SQL -select- error: %s\n", zErrMsg);
	  sqlite3_free(zErrMsg);
	}


	return;

}


