/*
 ============================================================================
 Name        : GI-learning.cpp
 Author      : Pietro Cottone and Gabriele Pergola
 Version     :
 Copyright   :
 Description :
 ============================================================================
 */

#include <ctime>
#include <iostream>
#include <map>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <string>
#include <exception>

#include "geoExp.h"

#define EXP_DIR "experiments"

#define MAX_BUFFER_SIZE 				256

#define USERS_IN_DB						180


#define PREFIX_NUMBER_LIMIT 			1000000				// Eventuale limite nel numero di prefissi analizzati per utente

#define MIN_NUMBER_POSITIVE_SAMPLES  	25
#define MIN_NUMBER_TEST_SAMPLES 		5

//#define K_FOLD_CROSS_VAL 				5

#include <chrono>

namespace fs=boost::filesystem;

using namespace std;


// An experiment is charatterizated by a single user
geoExp::geoExp(string db_path, int user, int min_prefixes_length, int max_prefixes_length, bool repetitions, int train_proportion, \
							  bool alg_edsm, bool alg_blues, double alpha, double delta)
{
	this->db_path = db_path;
	this->user = user;

	if(min_prefixes_length < 0 || min_prefixes_length > 9)
		min_prefixes_length = 1;													//TODO: da decidere se mettere a 1 facendo dei test
	this->min_prefixes_length = min_prefixes_length;


	if(max_prefixes_length <0 || max_prefixes_length > 9)
		max_prefixes_length = 9;
	this->max_prefixes_length = max_prefixes_length;


	this->no_repetitions_inside_strings = repetitions;


	this->training_proportion = train_proportion;

	this->test_proportion = 100 - train_proportion;

//	this->num_of_random_sets = num_random_set;

	// Numero di run del cross validation: è identificato univocamente dalla percentuale indicata come training set
	this->cross_val_run = ((double) 100.0 / (double)test_proportion);

	cout << "----> Number of run for CROSS VALIDATION: "<<cross_val_run << endl;


//  DA ELIMINARE SE FUNZIONA IL TUTTO
//	int tmp_min_n_positive_samples = MIN_NUMBER_POSITIVE_SAMPLES;
	//this->cross_val_run = (double) (test_proportion * tmp_min_n_positive_samples) / (double) 100.0;
//	if(this->cross_val_run == 0){
//		int cv_default = K_FOLD_CROSS_VAL;
//		cout << "Computed CVruns is 0! Forced to default value: "<< cv_default << endl;
//		this->cross_val_run = cv_default;
//	}


	edsm 		= alg_edsm;
	blueStar 	= alg_blues;
	if(blueStar){
		alpha_value = alpha;
		delta_value = delta;
	}
	else if(alpha != 0.0){
		cerr << "Alpha values selected without BlueStar algorithm"<<endl;
		exit(EXIT_FAILURE);
	}




	///////////////////////////////////////////////////////////////////////
	// Check or create the root "experiments" folder
	set_root_exp_folders();


	// Create the current experiment folder
	set_current_exp_folder();
}



//TODO: verificare che venga invoca il distruttore della classe "bluefringe"
geoExp::~geoExp(){};



// Find and set the execution folder, it's the base path
void geoExp::set_root_exp_folders()
{
	// Set the main directory: is the parent directory of execution folder
	exec_path = fs::current_path().parent_path().c_str();
	exec_path = exec_path + fs::path::preferred_separator;


	// Create the experiments folder, if not exists
	// From the execution folder, check and create an "experiments" folder.
	// Inside it, there are folders for every different length of the exmperiment
	// There are ".." beacuse exectuion folder is downside the main folder
	if( !fs::exists(exec_path+EXP_DIR) )
		root_exp_path = create_folder(exec_path, EXP_DIR, false);
	else
		root_exp_path = exec_path+EXP_DIR+"/";

	cout << "\"Experiments\" folder checked" << endl;
}


// Find and set the execution folder, it's the base path
void geoExp::set_current_exp_folder()
{

	// Create the current experiment folder
	// From the execution folder, check and create an "experiments" folder.
	// Inside it, there are folders for every different length of the exmperiment
	if(edsm)
		current_exp_folder = create_folder(root_exp_path, "user"+intTostring(user)+"_EDSM", true);
	else if(blueStar)
		current_exp_folder = create_folder(root_exp_path, "user"+intTostring(user)+"_BLUES", true);

	cout << "Current experiment directory is: "<<endl << current_exp_folder<<endl;

}



// Se "current_time" è true crea comunque una nuova cartella con l'orario di invocazione
string geoExp::create_folder(const string  base_path, const string new_folder, bool current_time){

	// Path of the new folder
	string new_path;

	// Define the folder for experiment results, if exists create a folder with modified name with current time
	new_path = base_path + new_folder;


	// move res_dir if it exists
	if(fs::exists(new_path) || current_time)
	{
		char append_time[MAX_BUFFER_SIZE];
		time_t rawtime = std::time(NULL);
		struct tm * timeinfo;
		time ( &rawtime );
		timeinfo = localtime ( &rawtime );
		strftime(append_time, MAX_BUFFER_SIZE, "%m_%d_%H_%M_%S", timeinfo);

		// Check if the folder exist yet, differntly change only the path and don't rename an inexistent folder
		if(fs::exists(new_path))
			fs::rename(new_path, new_path + "_" + append_time);
		else
			new_path = new_path + "_" + append_time;
	}


	// create res_dir
	fs::create_directory( new_path );


	// Update the exp_path for future use
	new_path = new_path + fs::path::preferred_separator;


	return new_path;
}




void geoExp::run_inference_accuracy()
{
	cout << "Selected algorithms are: " << endl;

	if(edsm)
		cout << "EDSM" << endl;
	if(blueStar)
		cout << "BlueStar"<<endl;

	cout << "Alpha of Blue*: "<< alpha_value << endl;

	cout << "Database path: "<<db_path<<endl;


	// Utenti analizzati
	string users[USERS_IN_DB];
	for(int i=0; i<USERS_IN_DB; ++i){
//		if(i <10)
//			users[i] = "00"+intTostring(i);
//		else if(i>=10 && i<100)
//			users[i] = "0"+intTostring(i);
//		else if(i>=100)
			users[i] = intTostring(i);
	}

	cout << "Lunghezza minima dei prefissi: " << min_prefixes_length << endl;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PREFIXES LENGHT
	for(int j=min_prefixes_length; j<max_prefixes_length; ++j)
	{
		string folder_current_prefix_len = "";
		set<string>::iterator it;
		int limit_prefixes_number = 0;

		cout << endl << endl << endl;
		cout << "///////////////////////////////////////////////////////" << endl;
		cout <<  "---------- PREFISSI DI LUNGHEZZA "+intTostring(j)+", utente "+users[user]+" -----------"<<endl;
		cout << "///////////////////////////////////////////////////////" << endl;


		// Open connection to DB
		geodb* mydb = new geodb(db_path);
		mydb->connect();

		// Prefixes of length 'j'
		set<string>* prefixes = mydb->get_prefixes_for_user(users[user], j);

		mydb->close();
		delete mydb;

		if(prefixes->size() == 0){
			cout << "No prefixes for this user!" << endl;
			continue;
		}


		// Make dir for all prefixes of length 'l'
		folder_current_prefix_len = create_folder(current_exp_folder, intTostring(j), false);


		// Istance the stat structure
		mystat statistics[prefixes->size()];

		for(int k=0; k<prefixes->size(); ++k)
		{
			statistics[k].percentage_positive_bluestar =  new double[cross_val_run];
			statistics[k].num_states_bluestar =  new int[cross_val_run];
			statistics[k].errore_rate_bluestar =  new double[cross_val_run];
			statistics[k].num_actual_merges_bluestar = new int[cross_val_run];
			statistics[k].num_heuristic_evaluations_bluestar =  new int[cross_val_run];

			for(int i=0; i<cross_val_run; ++i){
				statistics[k].percentage_positive_bluestar[i] =  -1;
				statistics[k].num_states_bluestar[i] =  -1;
				statistics[k].errore_rate_bluestar[i] =  -1;
				statistics[k].num_actual_merges_bluestar[i] = -1;
				statistics[k].num_heuristic_evaluations_bluestar[i] =  -1;
			}
		}


		// Cicla i PREFISSI
		cout << endl << prefixes->size() << " prefixes of length " << j <<", limit to ALL";



		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// ALL PREFIXES FOR CURRENT LENGTH
		for (it=prefixes->begin(); it!=prefixes->end() && limit_prefixes_number < PREFIX_NUMBER_LIMIT; ++it)
		{
			cout << endl << endl << endl << "***************************************************" << endl;
			cout <<  "------> Current Prefix: "<<*it << " - Utente "+users[user]<< " <--------"<<endl;
			cout << "***************************************************" << endl;

			limit_prefixes_number++;

			string*	test_set = NULL;
			geodb*	mydb = NULL;
			string	path_samples = "";
			string	path_training_data = "";
			string 	path_test_data = "";

			double succ_rate = -1;				// Success rate for current prefix


			// Current prefix
			string current_prefix = *it;

			statistics[limit_prefixes_number-1].prefix	= current_prefix;

			// *********************************
			//     WRITE MINITRAJECTORIES FILE
			// *********************************
			// Open connection to DB
			mydb = new geodb(db_path);
			mydb->connect();

			// Path per scrivere le minitraiettorie come samples, per il prefisso considerato
			path_samples		= folder_current_prefix_len + current_prefix;
			path_training_data	= path_samples + "-" + users[user] + "-samples-CV0.txt";
			path_test_data 		= path_samples + "-" + users[user] + "-test_samples-CV0.txt";

			int dim_test = 0;

			try
			{
				// Scrivi le ministraiettorie di training su file di testo e quelle di test nella variabile locale test_set (puntatore)
				dim_test = write_minitraj_from_db_like_samples_TRAINTEST_TESTSET(users[user], current_prefix, path_samples);
			}
			catch (const char* msg )
			{
				cout << msg << endl;
				cout << " >-------------------------------< "<< endl;
				mydb->close();
				delete mydb;

				continue;
			}

			mydb->close();
			delete mydb;



			////////////////////////////////////////////////////////////////////////////////////////////////////////
			if(edsm)
			{
				// *********************
				// 	    EDSM algorithm
				// *********************
				cout << endl << "******** EDSM "+current_prefix+"  ********" << endl;


				// Read positive and negative samples
				gi::edsm* edsm_exe = new gi::edsm(path_training_data.c_str());


				// Start inference
				gi::dfa* EDSM_dfa = edsm_exe->run(folder_current_prefix_len+current_prefix+"-");



				// *********************
				// 	    EDSM Statistics
				// *********************



				// Statistiche di generalizzazione
				if(dim_test != 0)
				{
					succ_rate = make_generalization_test(EDSM_dfa, path_test_data.c_str()/*, dim_test*/);


					// Statistics
					if(dim_test == 0)
						succ_rate = 1;


					statistics[limit_prefixes_number-1].prefix	= current_prefix;
					statistics[limit_prefixes_number-1].num_states_edsm = EDSM_dfa->get_num_states();
					statistics[limit_prefixes_number-1].num_actual_merges_edsm = edsm_exe->get_actual_merge();
					statistics[limit_prefixes_number-1].num_heuristic_evaluations_edsm = edsm_exe->get_heuristic_merge();

					statistics[limit_prefixes_number-1].percentage_positive_edsm =  succ_rate;

					cout << "Tasso di generalizzazione di EDSM: "<<succ_rate*100<<"%"<<endl;

				}
				else
					cout << "XXX Caso banale: una sola stringa positiva XXX"<<endl;


				// *********************
				// 	    EDSM print
				// *********************

				// DOT without alphabet
				string dotEdsmpath = folder_current_prefix_len+current_prefix+"-DOTedsm.dot";
				EDSM_dfa->print_dfa_dot("EDSM", dotEdsmpath.c_str());


				// DOT with alphabet
				string dotEdsmpath_alf = folder_current_prefix_len+current_prefix+"-DOTedsmALF.dot";
				EDSM_dfa->print_dfa_dot_mapped_alphabet("EDSM", dotEdsmpath_alf.c_str());
				cout << "EDSM  DFA path: "<<dotEdsmpath_alf << endl;


				// DFA in txt file
				string txtEDSMpath_alf_A = folder_current_prefix_len+current_prefix+"-TXTedsmALF.txt";
				EDSM_dfa->print_dfa_in_text_file(txtEDSMpath_alf_A);
				cout << "EDSM DFA TXT path: "<<txtEDSMpath_alf_A << endl;


				// Delete
				if(edsm_exe != NULL)
					delete edsm_exe;
				if(EDSM_dfa != NULL)
					delete EDSM_dfa;
			}




			if(blueStar)
			{
					for(int i=0; i<cross_val_run; ++i)
					{
						// *********************
						// 	 BLUESTAR algorithm
						// *********************
						cout << endl<< "********  BLUESTAR "+current_prefix+"-cv: "+intTostring(i)+" *********" << endl;
						gi::dfa* BLUESTAR_dfa;

						path_training_data	= path_samples + "-" + users[user] + "-samples-CV"+intTostring(i)+".txt";
						path_test_data 		= path_samples + "-" + users[user] + "-test_samples-CV"+intTostring(i)+".txt";


						// Read positive and negative samples
						gi::blueStar* bluestar_exe = new gi::blueStar(path_training_data.c_str(), alpha_value, delta_value);


						// Start inference
						try
						{

							BLUESTAR_dfa = bluestar_exe->run(folder_current_prefix_len+current_prefix+"-");

						}
						catch( const char* msg ){	// NB: Questo potrebbe sfalzare la corrispondenza tra i valori nel file STATISTICS e i prefissi a cui si riferiscono
							cout << msg << "; "<<endl;
							cout << " >---------------------------------------< " << endl;
							continue;
						}



						// *********************
						// 	 BLUESTAR Statistics
						// *********************

						// Statistiche di generalizzazione
						if(dim_test != 0)
						{
							succ_rate = make_generalization_test(BLUESTAR_dfa, path_test_data.c_str()/*, dim_test*/);


							// Statistics
							if(dim_test == 0)
								succ_rate = 1;


							statistics[limit_prefixes_number-1].prefix = current_prefix;
							statistics[limit_prefixes_number-1].percentage_positive_bluestar[i] =  succ_rate;
							statistics[limit_prefixes_number-1].num_states_bluestar[i] = BLUESTAR_dfa->get_num_states();
							statistics[limit_prefixes_number-1].errore_rate_bluestar[i] = bluestar_exe->get_error_rate_final_dfa();
							statistics[limit_prefixes_number-1].num_actual_merges_bluestar[i] = bluestar_exe->get_actual_merge();
							statistics[limit_prefixes_number-1].num_heuristic_evaluations_bluestar[i] = bluestar_exe->get_heuristic_merge();

							cout << "Tasso di generalizzazione di BLUESTAR: "<<succ_rate*100<<"%"<<endl;
							cout << "Error-rate sulle stringhe positive: "<< bluestar_exe->get_error_rate_final_dfa() << endl;


						}
						else
							cout << "XXX Caso banale: una sola stringa positiva XXX"<<endl;





						// *********************
						// 	    BLUESTAR  print
						// *********************

						// Print transition table of the inferred automaton
						//BLUESTAR_dfa.print_dfa_ttable("- BlueStar dfa -");

						// Create dot figure for the inferred automaton
						string dotBlueStarpath_alf = folder_current_prefix_len+current_prefix+"-DOTbluestarALF-CV"+intTostring(i)+".dot";
						BLUESTAR_dfa->print_dfa_dot_mapped_alphabet("BlueStar", dotBlueStarpath_alf.c_str());
						cout << "BLUSTAR DFA path: "<<dotBlueStarpath_alf << endl;

						// It prints the inferred automaton in a text file
						string txtBlueStarpath_alf_A = folder_current_prefix_len+current_prefix+"-CV"+intTostring(i)+"-TXTbluestarALF.txt";
						BLUESTAR_dfa->print_dfa_in_text_file(txtBlueStarpath_alf_A);
						cout << "BLUSTAR DFA TXT path: "<<txtBlueStarpath_alf_A << endl;


						// free allocated memory
						if(bluestar_exe!=NULL)
							delete bluestar_exe;
						if(BLUESTAR_dfa != NULL)
							delete BLUESTAR_dfa;

					} // CROSS VALL FOR

			}

		}


		//Statistics for a fixed legnth of prefix
		if(edsm)
			write_statistics_files(statistics, prefixes->size(), intTostring(user), true, 0);

		for(int i=0; i<cross_val_run; ++i)
			if(blueStar)
				write_statistics_files(statistics, prefixes->size(), intTostring(user), false, i);


		stat_final_results(statistics, prefixes->size(), intTostring(user), edsm);

		stat_final_results_minimal(statistics, prefixes->size(), intTostring(user), edsm);


		prefixes->clear();
		delete prefixes;

	}

}







// Estrae le statistiche di generalizzazione dell'automa
// (a differenza di quello implementato dentro edsm.cpp,
//  questo ha in input le stringhe di test non ancora prelaborate)
double geoExp::make_generalization_test(gi::dfa* finaldfa, const char * file_path/*,  int dim_test*/)
{
	cout << "**** TEST DI GENERALIZZAZIONE ****" << endl;

	int* wp;
	int dim_testset;

	int total_weigth = 0;


	// Read test set from file
	string* test_set = read_testsamples_leaving_geohash_encoding(file_path, dim_testset, wp);

	cout << "Stringhe di test lette dal file: "<<dim_testset << endl;

	int successful = 0;

	for(int i=0; i<dim_testset; ++i)
	{
		// Stringa (eventualmente) senza ripetizioni
		// (devo fare il mapping invero dell'alfabato)
		//cout << "Stringa letta: "<< test_set[i] << ", peso: "<< wp[i] << endl;
		string test_string_tmp = test_set[i];
		vector<SYMBOL> test_string;

		//string test_string_tmp = add_space_between_characters_delete_repetitions(test_set[i]);


		total_weigth += wp[i];

		// Ricostruisco la stringa carattere per carattere facendo il mapping inverso dell'alfabeto
		for(unsigned int j=0; j<test_string_tmp.length(); ++j)
		{
			if(test_string_tmp[j] == ' ')
				continue;


			short int mapped_index = geo_alph.find_first_of(test_string_tmp[j]);

			if(mapped_index == -1){
				cerr << "The " << test_string_tmp[j] << " symbol is not inside the Geohash alphabet"<<endl;
				exit(EXIT_FAILURE);
			}

			test_string.push_back(mapped_index);
		}

		//cout << "Stringa iniziale: " << test_string_tmp << endl;
		//cout << "Stringa in test: "<<test_string<<endl;


		if(finaldfa->membership_query(test_string))
		{
			successful += wp[i];

			//cout << "ACC: ";
			//cout << test_string << endl;
		}
		else
		{
			//cout << "NONACC: ";
			//cout << test_string << endl;
		}
	}

	double succ_rate = (double) successful / (double) total_weigth;

	cout << "**** End of generalization test ****"<<endl;

	return succ_rate;
}



// Scrive su file il training set e setta nel vettore "test_set" le stringhe di test
// Ritorna la dimensione del test_set, adoperato per poter fare esternamente i test
// Scrito solo il training perché è sufficiene all'inferenza
// (se vuoi puoi scrivere anche quelle di test nel file, basta copiare e adattare l'ultima riga della funzione)
int geoExp::write_minitraj_from_db_like_samples_TRAINTEST_TESTSET(string user,string prefix, string path_samples)
{
	//int  *wp, *wn;
	//vector<string>::iterator it;

	vector<string> test_set[cross_val_run];
	vector<string> positive_training_set[cross_val_run];


	// Open connection
	geodb* mydb = new geodb(db_path);
	mydb->connect();


	// Positive Samples - minitrajectories for user and prefix
	vector<string>* positive_samples = mydb->get_minitraj_for_prefix_and_user(user, prefix);


	// Negative Samples - minitrajectories for all users, except 'user', and prefix
	vector<string>* negative_samples = mydb->get_minitraj_for_prefix_and_allUsers_EXCEPT_user(user, prefix);


	int dim_positive = positive_samples->size();

	cout << "Positive: "<<dim_positive << endl;

	if(dim_positive < MIN_NUMBER_POSITIVE_SAMPLES)
	{
		// Free memory
		for(int i=0; i< cross_val_run; ++i){
			positive_training_set[i].clear();
			test_set[i].clear();
		}

		vector<string>().swap(*positive_samples);
		vector<string>().swap(*negative_samples);
		positive_samples->clear();
		negative_samples->clear();
		delete positive_samples;
		delete negative_samples;

		mydb->close();
		delete mydb;

		throw "Size of dataset too small ";
	}


	cout << " - Scrivo su file i samples -" << endl;
	cout << "Prefisso: "<<prefix<<endl;
	cout << "Numero sample positivi: "<<positive_samples->size()<<endl;
	cout << "Numero sample negativi: "<<negative_samples->size()<<endl;



	// ****************************************
	// Divido in TRAINING SET e TEST SET 80-20
	// ****************************************

	// Inizzializzo srand
	srand(time(NULL));


	random_shuffle(positive_samples->begin(), positive_samples->end());


	// Calcolo la dimensione del training set e del test set
	int dim_train = ceil( (double) (training_proportion * dim_positive) / (double) 100);
	int dim_test  = dim_positive - dim_train;

	if(dim_positive == 1){
		dim_train = 1;
		dim_test = 0;
	}

	cout << "Tot previste pos: "<<dim_positive<<" | "<<"train: "<<dim_train<<" - test: "<<dim_test<<endl;


	if(dim_test < MIN_NUMBER_TEST_SAMPLES)
	{
		// Free memory
		for(int i=0; i< cross_val_run; ++i){
			vector<string>().swap(positive_training_set[i]);
			vector<string>().swap(test_set[i]);
			positive_training_set[i].clear();
			test_set[i].clear();
		}

		vector<string>().swap(*positive_samples);
		vector<string>().swap(*negative_samples);
		positive_samples->clear();
		negative_samples->clear();
		delete positive_samples;
		delete negative_samples;

		mydb->close();
		delete mydb;

		throw "Size of dataset too small ";
	}



	////////////////////////////////////////////////////////////////////////
	// Creo il set di TEST e elimino dal training
	if(dim_test != 0)
	{
		for(int i=0; i< cross_val_run; ++i)
		{
			for(auto it=positive_samples->begin(); it != positive_samples->end(); ++it)
			{
				if(test_set[i].size() < dim_test)
					test_set[i].push_back(*it);
				else
					positive_training_set[i].push_back(*it);
			}

			rotate( positive_samples->begin(), positive_samples->begin() + test_set[i].size(),  positive_samples->end());

			// TODO: togliere eventuali duplicati


			cout << "Train finale: "<<positive_training_set[i].size()<<" - testset finale: "<< test_set[i].size() <<endl;
		}
	}




	////////////////////////////////////////////////////////////////////////

	// Scrivo su file
	// (è qui dentro che eventualmente tolgo le RIPETIZIONI interne ad una stringa)
	cout << "Scritture su file dei samples..."<<endl;

	for(int i=0; i<cross_val_run; ++i)
	{
		// Scrivo su file il TRAINIG SET
		string path_training_data = path_samples+ "-"+user+"-samples-CV"+intTostring(i)+".txt";
		write_minitrajectories_as_training_set(&positive_training_set[i] , negative_samples, path_training_data.c_str());


		// Scrivo su file il TEST SET
		string path_test_data = path_samples + "-"+user+"-test_samples-CV"+intTostring(i)+".txt";
		write_minitrajectories_as_test_set(&test_set[i], path_test_data.c_str());
	}


	// Free memory
	for(int i=0; i< cross_val_run; ++i){
		vector<string>().swap(positive_training_set[i]);
		vector<string>().swap(test_set[i]);
		positive_training_set[i].clear();
		test_set[i].clear();
	}

	vector<string>().swap(*positive_samples);
	vector<string>().swap(*negative_samples);
	positive_samples->clear();
	negative_samples->clear();
	delete positive_samples;
	delete negative_samples;

	mydb->close();
	delete mydb;

	return dim_test;
}




//// Scrivo tutto il training set nel file. Usato per la similarità tra utenti
//void geoExp::write_minitraj_from_db_like_samples_ALL_TRAININGSET(string user,string prefix, string path_samples)
//{
//
//	// Open connection
//	geodb* mydb = new geodb(db_path);
//	mydb->connect();
//
//
//	// Positive Samples - minitrajectories for user and prefix
//	vector<string>* positive_samples = mydb->get_minitraj_for_prefix_and_user(user, prefix);
//
//
//	// Negative Samples - minitrajectories for all users, except 'user', and prefix
//	vector<string>* negative_samples = mydb->get_minitraj_for_prefix_and_allUsers_EXCEPT_user(user, prefix);
//
//
//	if(positive_samples->size() < MIN_NUMBER_POSITIVE_SAMPLES || negative_samples->size() < MIN_NUMBER_POSITIVE_SAMPLES )
//	{
//		// Free memory
//		vector<string>().swap(*positive_samples);
//		vector<string>().swap(*negative_samples);
//		positive_samples->clear();
//		negative_samples->clear();
//		delete positive_samples;
//		delete negative_samples;
//
//		mydb->close();
//		delete mydb;
//
//		throw "Size of dataset too small ";
//	}
//
//
//	cout << endl << "Scrivo l'intero trainingset per utente "<< user << ", prefisso: "<<prefix<< endl;
//	cout << "Prefisso: "<<prefix<<endl;
//	cout << "Numero sample positivi: "<<positive_samples->size()<<endl;
//	cout << "Numero sample negativi: "<<negative_samples->size()<<endl;
//
//
//	////////////////////////////////////////////////////////////////////////
//	// Scrivo su file
//	// (è qui dentro che eventualmente tolgo le RIPETIZIONI interne ad una stringa)
//
//	string path_training_data = path_samples+ "-"+user+"-samples.txt";
//	write_minitrajectories_as_training_set(positive_samples, negative_samples, path_training_data.c_str());
//
//
//
//	// Free memory
//	vector<string>().swap(*positive_samples);
//	vector<string>().swap(*negative_samples);
//	positive_samples->clear();
//	negative_samples->clear();
//	delete positive_samples;
//	delete negative_samples;
//
//	mydb->close();
//	delete mydb;
//
//}




// Scrive su file le minitraiettorie giˆ estratte (p_samples) per l'utente ed un prefisso particolare
void geoExp::write_minitrajectories_as_training_set(vector<string>* p_orig_samples, vector<string>* n_orig_samples, const char * file_path)
{

	// Keep a local copy for working
	vector<string> p_samples(*p_orig_samples);
	vector<string> n_samples(*n_orig_samples);


	// Insert space between single characters in a string. This is the iterator
	vector<string>::iterator it;


	// Positive samples
	for (it=p_samples.begin(); it!=p_samples.end(); ++it)
	{
		// Add spaces between characters
		string new_string = add_space_between_characters_delete_repetitions(*it);
		new_string = trimRight(new_string);

		(*it) = new_string;
	}


	// Negative samples
	int count =0;
	bool non_empty_intersection = false;
	for (it=n_samples.begin(); it!= n_samples.end();)
	{

		// Add spaces between characters
		string new_string = add_space_between_characters_delete_repetitions(*it);
		new_string = trimRight(new_string);


		if( find( p_samples.begin(), p_samples.end(), new_string) != p_samples.end() ){
			//cout << "Doppione tra Sample positivo e negativo: "<<new_string<< endl;
			non_empty_intersection = true;
			it = n_samples.erase(it);									// Per ogni run cambia il numero di volte che entra perché il test set è casuale
		} else {

			(*it) = new_string;
			++it;
		}


		// Add new positive samples to map
		//samples[new_string] = 0;
	}


	cout << "Dopo il aver tolto ripetizioni intrastringhe e tolto samples comuni tra pos e neg, i neg sono: "<<n_samples.size() << endl;

	if(non_empty_intersection)
		cout << "Ci sono stati doppioni tra pos e neg"<<endl;



	//////////////////////////////
	// Add to map with weights for samples
	map<string, int> weights_p;
	map<string, int> weights_n;

	for(auto i=p_samples.begin(); i!=p_samples.end(); ++i){
		auto j = weights_p.begin();

		if((j = weights_p.find(*i)) != weights_p.end())				// Controllo che sia definito uno stato per quando entra dopo lambda un elemento dell'alfabeto
			j->second++;
		else
			weights_p[*i] = 1;
	}

	for(auto i=n_samples.begin(); i!=n_samples.end(); ++i){
		auto j = weights_n.begin();

		if((j = weights_n.find(*i)) != weights_n.end())				// Controllo che sia definito uno stato per quando entra dopo lambda un elemento dell'alfabeto
			j->second++;
		else
			weights_n[*i] = 1;
	}




	// **********************
	//   WRITE FILE SAMPLES
	// **********************

	cout << "Scrittura su file del training set (positivi e negativi)"<<endl;


	// Write file with positive and negative samples
	ofstream myfile;
	myfile.open(file_path);


	// Write alphabet size
	myfile << "32" << "\n";

	// Write Geohash alphabet symbols
	myfile << "$ ";									// Empty symbol
	for(int i=0; i<32; ++i)
		myfile << geo_alph[i] << " ";
	myfile << "\n";

	// Write positive samples
	for (auto it=weights_p.begin(); it!=weights_p.end(); ++it)
		if( it->first.length() < MAX_LENGTH_SAMPLES_POS )
			myfile << "+ "+intTostring(it->second)+" "+it->first+"\n";

	// Write negative samples
	for (auto it=weights_n.begin(); it!=weights_n.end(); ++it)
		if( it->first.length() < MAX_LENGTH_SAMPLES_POS )
			myfile << "- "+intTostring(it->second)+" "+it->first+"\n";


	cout << "Su file sono presenti "<<weights_p.size()<<" samples positivi e "<<weights_n.size()<<" negativi"<<endl;

//	// Write positive samples
//	for(It p1=samples.begin(); p1!=samples.end(); ++p1)
//		if((*p1).second  == 1)
//			if( (*p1).first.size() < MAX_LENGTH_SAMPLES_POS)
//				myfile << "+ "+(*p1).first+"\n";
//
//	// Write negative samples
//	for(It p1=samples.begin(); p1!=samples.end(); ++p1)
//		if((*p1).second  == 0)
//			if( (*p1).first.size() < MAX_LENGTH_SAMPLES_NEG)
//				myfile << "- "+(*p1).first+"\n";


	myfile.close();

}


void geoExp::write_minitrajectories_as_test_set(vector<string>* test_samples, const char * file_path)
{

	// Keep a local copy for working
	//vector<string> p_samples(*test_orig_samples);


	// Positive samples
	for (auto it=test_samples->begin(); it!=test_samples->end(); ++it)
	{
		// Add spaces between characters
		string new_string = add_space_between_characters_delete_repetitions(*it);
		new_string = trimRight(new_string);

		(*it) = new_string;
	}


	//////////////////////////////
	// Add to map with weights for samples
	map<string, int> weights_p;

	for(auto i=test_samples->begin(); i!=test_samples->end(); ++i){
		auto j = weights_p.begin();

		if((j = weights_p.find(*i)) != weights_p.end())				// Controllo che sia definito
			j->second++;
		else
			weights_p[*i] = 1;
	}


	// **********************
	//   WRITE FILE SAMPLES
	// **********************

	cout << "Scrittura su file del test set"<<endl;


	// Write file with positive and negative samples
	ofstream myfile;
	myfile.open(file_path);


	// Write alphabet size
	myfile << "32" << "\n";

	// Write Geohash alphabet symbols
	myfile << "$ ";									// Empty symbol
	for(int i=0; i<32; ++i)
		myfile << geo_alph[i] << " ";
	myfile << "\n";

	// Write positive samples
	for (auto it=weights_p.begin(); it!=weights_p.end(); ++it)
		if( it->first.length() < MAX_LENGTH_SAMPLES_POS )
			myfile << "+ "+intTostring(it->second)+" "+it->first+"\n";


	cout << "Su file sono presenti "<<weights_p.size() << " test samples"<<endl;


	myfile.close();

}






// Aggiunge uno spazio tra un carattere e il successivo delle minitraiettorie, per la compatibilità del formato stringhe
string geoExp::add_space_between_characters_delete_repetitions(string old_string)
{
	//cout << "ORIGINALE: " << old_string<<endl;

	string new_string = "";
	int new_length = 0;

	if(!no_repetitions_inside_strings)
	{

		new_length = (  old_string.length() * 2 ) - 1;
		//cout << "Lunghezza nuova stringa: "<<new_length<<endl;

		int index_char = 0;

		char last_char = 'a';

		for(int i=0; i<new_length; ++i)
		{
		   if(i % 2 == 0){

			   new_string = new_string+(old_string)[index_char];
			   last_char = (old_string)[index_char];
			   //cout << "elemento: "<<(old_string)[count]<<endl;
			   index_char++;

		   }else{
			   new_string = new_string+" ";
		   }
		}

	}
	else		// Tolgo qui le RIPETIZIONI interne ad una stringa
	{

		char last_char = 'a';				// Sono sicuro che 'a' non è presente nel geohash

		bool spazio = false;
		for(unsigned int i=0; i<old_string.length(); ++i)
		{
			if(!spazio)
			{
			   if(last_char == (old_string)[i])
				   continue;

			   new_string = new_string+(old_string)[i];
			   last_char = (old_string)[i];

			   spazio = true;
		   }else{
			   new_string = new_string+" ";
			   spazio = false;
		   }
		}
	}

	 //cout << "Nuova stringa: "<<new_string<<endl;

	return new_string;
}


string* geoExp::read_testsamples_leaving_geohash_encoding(const char * /*path*/ path_samples, int &dim_positive, int* &wp)
{
	cout << "Reading strings from txt file: "<<endl;
	int cp = 0;														// Numero di stringhe positive del linguaggio
	char ch;

	char null_symbol;

	map<char, int> mapped_alphabet;
	char* inverse_mapped_alphabet;
	int dim_alphabet;
	//int dim_positive;

	cout << path_samples << endl;

	fstream fin(path_samples, fstream::in);

	if(!fin){
		cerr<<"An error occurred in opening file :("<<endl;
		exit(EXIT_FAILURE);
	}


	// Faccio in conteggio previo del numero di stringhe positive e negative presenti nel txt
	while (fin >> noskipws >> ch) {
		if(ch == '\n')
			continue;
		else if(ch == '+')
			cp++;
	}
	dim_positive = cp;


	string* positive = new string[cp];


	wp	= new int[cp];
	for(int i=0; i<cp; ++i)
		wp[i] = 0;

	cout << intTostring(cp) + " positivi"<<endl;

	int flagcp = 0;
	bool casopositive = false;
	bool primap = true;

	ifstream infile(path_samples);

	bool first = true;
	bool second = false;
	string line;

	while (getline(infile, line))
	{
	    istringstream iss(line);
	    int a;
	    string n;


	    // Read first line for dim alphabet
	    if(first)
	    {
	    	if (!(iss >> a)) { break; } // error
	    	dim_alphabet = a;
	    	//cout << "dimensione alfabeto " << a << endl;
	    	first = false;
	    	second = true;

	    	continue;
	    }


	    // Read second line for alphabet symbol
	    if(second)
	    {
	    	inverse_mapped_alphabet = new char[dim_alphabet];

	    	int counter=-1;
	    	while (iss >> n){
	    		if(counter==-1){
	    			null_symbol = (char) n[0];

	    			++counter;
	    			continue;
	    		}else if(counter>=dim_alphabet)
	    			break;

	    		mapped_alphabet[(char) n[0]] = counter;
	    		inverse_mapped_alphabet[counter++]=n[0];
	    	}

	    	// Alphabet
	    	if(counter!= dim_alphabet){
	    		cerr<<"Error in reading example: number of red alphabet symbols mismatches with the declared one!"<<endl;
	    		cerr<<"Expected symbols: "<<dim_alphabet<<endl;
	    		cerr<<"Reed symbols: "<<counter<<endl;


	    		exit(EXIT_FAILURE);
	    	}

	    	// alphabet ok ;)
	    	second= false;
	    }


	    bool weight = true;
	    bool negative_to_neglect = false;

	    // Read remaining lines. NB:  "iss >> n" read set of characters since next space!
		while (iss >> n)
		{
			if( !n.compare("+") ){

				weight = true;
				casopositive = true;
				if(primap){												// Se è il primo caso evito l'incremento
					primap =false;
					continue;
				}
				flagcp++;
				negative_to_neglect = false;
				continue;

			} else if(  !n.compare("-") ){
				negative_to_neglect = true;
				continue;
			}

			// Se sono in una stringa negativa devo saltare tutto fino alla prossima positiva
			if(negative_to_neglect)
				continue;

			// se la stringa è vuota, non è necessario aggiungere nulla
			if(((char) n[0]) == null_symbol)
				continue;

			// Ho letto il peso, lo aggiungo ai pesi
			if(weight){
				weight = false;

				if(casopositive)
					wp[flagcp] = stringToint(n);

			} else {

				//int tmp = mapped_alphabet[(char) n[0]];

				if(casopositive)
					positive[flagcp] = positive[flagcp] +  n;
			}

		}
	}

	return positive;
}









///////////////////////////////////////////////////////
// STATISTICS

void geoExp::write_statistics_files(mystat* userstat, int n_prefixes, string utente, bool is_edsm, int curr_run_cross_val)
{

	stat_write_generalization_rate(userstat, n_prefixes, utente, is_edsm, curr_run_cross_val);

	stat_write_num_states(userstat, n_prefixes, utente, is_edsm, curr_run_cross_val);

	stat_write_error_rate_on_trainigset(userstat, n_prefixes, utente, is_edsm, curr_run_cross_val);

	stat_actual_merges(userstat, n_prefixes, utente, is_edsm, curr_run_cross_val);

	stat_heuristic_evaluations(userstat, n_prefixes, utente, is_edsm, curr_run_cross_val);
}



void geoExp::stat_write_generalization_rate(mystat* userstat, int n_prefixes, string utente, bool is_edsm, int curr_run_cross_val)
{
	string file_path = "";

	if(is_edsm)
		file_path = current_exp_folder + "STAT_GENERAL_EDSM.txt";
	else
		file_path = current_exp_folder+ "STAT_GENERAL_BLUESTAR"+intTostring(curr_run_cross_val)+".txt";

	cout << "Statistics file: "<<file_path << endl;


	// Write in file the positive e negative samples
	ofstream myfile;
	myfile.open(file_path.c_str(), ios::app);

	myfile << endl << endl << "Utente: "<< utente << " - Lunghezza prefisso: "<< intTostring(userstat[0].prefix.length()) << endl;
	myfile << "Generlizzation rate and positive error Blustar:" << endl;

	for(int i=0; i<n_prefixes; ++i)
	{
		if(is_edsm){

			if( userstat[i].percentage_positive_edsm != -1 )
				myfile << userstat[i].percentage_positive_edsm << endl;
			else
				myfile << "-" << endl;

		}
		else if ( userstat[i].percentage_positive_bluestar[curr_run_cross_val] != -1 )				//TODO: !?!?!?! METTI INVECE solo del primo elemento del vettore uno scorrimento
			myfile << userstat[i].percentage_positive_bluestar[curr_run_cross_val]<< endl;
		else
			myfile << "-" << endl;
	}

	myfile.close();
}



void geoExp::stat_write_num_states(mystat* userstat, int n_prefixes, string utente, bool is_edsm, int curr_run_cross_val)
{
	string file_path = "";

	if(is_edsm)
		file_path = current_exp_folder + "STAT_NUM_STATES_EDSM.txt";
	else
		file_path = current_exp_folder+ "STAT_NUM_STATES_BLUESTAR.txt"+intTostring(curr_run_cross_val)+".txt";

	cout << "Statistics file: "<<file_path << endl;


	// Write in file the positive e negative samples
	ofstream myfile;
	myfile.open(file_path.c_str(), ios::app);

	myfile << endl << endl << "Utente: "<< utente << " - Lunghezza prefisso: "<< intTostring(userstat[0].prefix.length()) << endl;
	myfile << "Number of states:" << endl;

	for(int i=0; i<n_prefixes; ++i)
	{
		if(is_edsm){

			if( userstat[i].num_states_edsm != -1 )
				myfile << userstat[i].num_states_edsm << endl;
			else
				myfile << "-" << endl;

		}
		else if ( userstat[i].num_states_bluestar[curr_run_cross_val] != -1 )
			myfile << userstat[i].num_states_bluestar[curr_run_cross_val]<< endl;
		else
			myfile << "-" << endl;
	}

	myfile.close();
}



void geoExp::stat_write_error_rate_on_trainigset(mystat* userstat, int n_prefixes, string utente, bool is_edsm, int curr_run_cross_val)
{
	string file_path = "";

	if(is_edsm)
		return;
	else
		file_path = current_exp_folder+ "STAT_ERR_RATE_TRAIN_BLUESTAR.txt"+intTostring(curr_run_cross_val)+".txt";

	cout << "Statistics file: "<<file_path << endl;


	// Write in file the positive e negative samples
	ofstream myfile;
	myfile.open(file_path.c_str(), ios::app);

	myfile << endl << endl << "Utente: "<< utente << " - Lunghezza prefisso: "<< intTostring(userstat[0].prefix.length()) << endl;
	myfile << "Error state on training set by BlueStar:" << endl;

	for(int i=0; i<n_prefixes; ++i)
	{
		if ( userstat[i].errore_rate_bluestar[curr_run_cross_val] != -1 )
			myfile << userstat[i].errore_rate_bluestar[curr_run_cross_val] << endl;
		else
			myfile << "-" << endl;
	}

	myfile.close();
}



void geoExp::stat_actual_merges(mystat* userstat, int n_prefixes, string utente, bool is_edsm, int curr_run_cross_val)
{
	string file_path = "";

	if(is_edsm)
		file_path = current_exp_folder + "STAT_ACTUAL_MERGES_EDSM.txt";
	else
		file_path = current_exp_folder+ "STAT_ACTUAL_MERGES_BLUESTAR.txt"+intTostring(curr_run_cross_val)+".txt";

	cout << "Statistics file: "<<file_path << endl;


	// Write in file the positive e negative samples
	ofstream myfile;
	myfile.open(file_path.c_str(), ios::app);

	myfile << endl << endl << "Utente: "<< utente << " - Lunghezza prefisso: "<< intTostring(userstat[0].prefix.length()) << endl;
	myfile << "Number of actual merges:" << endl;

	for(int i=0; i<n_prefixes; ++i)
	{
		if(is_edsm){

			if( userstat[i].num_actual_merges_edsm != -1 )
				myfile << userstat[i].num_actual_merges_edsm << endl;
			else
				myfile << "-" << endl;

		}
		else if ( userstat[i].num_actual_merges_bluestar[curr_run_cross_val] != -1 )
			myfile << userstat[i].num_actual_merges_bluestar[curr_run_cross_val]<< endl;
		else
			myfile << "-" << endl;
	}

	myfile.close();
}



void geoExp::stat_heuristic_evaluations(mystat* userstat, int n_prefixes, string utente, bool is_edsm, int curr_run_cross_val)
{
	string file_path = "";

	if(is_edsm)
		file_path = current_exp_folder + "STAT_HEURISTIC_EVALUATIONS_EDSM.txt";
	else
		file_path = current_exp_folder+ "STAT_HEURISTIC_EVALUATIONS_BLUESTAR.txt"+intTostring(curr_run_cross_val)+".txt";

	cout << "Statistics file: "<<file_path << endl;


	// Write in file the positive e negative samples
	ofstream myfile;
	myfile.open(file_path.c_str(), ios::app);

	myfile << endl << endl << "Utente: "<< utente << " - Lunghezza prefisso: "<< intTostring(userstat[0].prefix.length()) << endl;
	myfile << "Number of heuristic evaluetions:" << endl;

	for(int i=0; i<n_prefixes; ++i)
	{
		if(is_edsm){

			if( userstat[i].num_heuristic_evaluations_edsm != -1 )
				myfile << userstat[i].num_heuristic_evaluations_edsm << endl;
			else
				myfile << "-" << endl;

		}
		else if ( userstat[i].num_heuristic_evaluations_bluestar[curr_run_cross_val] != -1 )
			myfile << userstat[i].num_heuristic_evaluations_bluestar[curr_run_cross_val]<< endl;
		else
			myfile << "-" << endl;
	}

	myfile.close();
}



void geoExp::stat_final_results(mystat* userstat, int n_prefixes, string utente, bool is_edsm)
{
	string file_path_BLUESTAR = "";



	if(cross_val_run == 0){
		cerr << "Cross Validation not specificated" << endl;
		return;
	}


	string file_path_EDSM = current_exp_folder + "STAT_FINAL_RESULTS_EDSM.txt";

	file_path_BLUESTAR = current_exp_folder+ "STAT_FINAL_RESULTS_BLUESTAR.txt";

	cout << "Statistics file: "<<file_path_BLUESTAR << endl;


	// Write in file the positive e negative samples
	ofstream myfile;
	myfile.open(file_path_BLUESTAR.c_str(), ios::app);

	myfile << endl << endl << "Utente: "<< utente << " - Lunghezza prefisso: "<< intTostring(userstat[0].prefix.length()) << endl;
	myfile << "Media e varianza del Cross Validation." << endl;


	// Accumulatore per i dati per avere un valore complessivo rispetto a tutti i prefissi della lunghezza fissata - BLUESTAR
	boost::accumulators::accumulator_set<double, boost::accumulators::features<boost::accumulators::tag::mean, boost::accumulators::tag::variance> > value_resulting_from_all_prefixes_together[5];

	// Accumulatore per i dati per avere un valore complessivo rispetto a tutti i prefissi della lunghezza fissata - BLUESTAR
	boost::accumulators::accumulator_set<double, boost::accumulators::features<boost::accumulators::tag::mean, boost::accumulators::tag::variance> > EDSM_value_resulting_from_all_prefixes_together[4];


	for(int i=0; i<n_prefixes; ++i)
	{

		if(is_edsm && userstat[i].num_states_edsm != -1){
			EDSM_value_resulting_from_all_prefixes_together[0] ( userstat[i].num_states_edsm );
			EDSM_value_resulting_from_all_prefixes_together[1] ( userstat[i].percentage_positive_edsm );
			EDSM_value_resulting_from_all_prefixes_together[2] ( userstat[i].num_actual_merges_edsm );
			EDSM_value_resulting_from_all_prefixes_together[3] ( userstat[i].num_heuristic_evaluations_edsm );
		}


		if ( userstat[i].num_heuristic_evaluations_bluestar[0] != -1 )
		{
			// Accumulatore per i dati
			boost::accumulators::accumulator_set<double, boost::accumulators::features<boost::accumulators::tag::mean, boost::accumulators::tag::variance> > acc[5];


			// Carico i file nell'accumulators
			for(int k=0; k<cross_val_run; ++k){
				acc[0] ( userstat[i].num_states_bluestar[k] );
				acc[1] ( userstat[i].percentage_positive_bluestar[k] );
				acc[2] ( userstat[i].errore_rate_bluestar[k] );
				acc[3] ( userstat[i].num_actual_merges_bluestar[k] );
				acc[4] ( userstat[i].num_heuristic_evaluations_bluestar[k] );

				value_resulting_from_all_prefixes_together[0] ( userstat[i].num_states_bluestar[k] );
				value_resulting_from_all_prefixes_together[1] ( userstat[i].percentage_positive_bluestar[k] );
				value_resulting_from_all_prefixes_together[2] ( userstat[i].errore_rate_bluestar[k] );
				value_resulting_from_all_prefixes_together[3] ( userstat[i].num_actual_merges_bluestar[k] );
				value_resulting_from_all_prefixes_together[4] ( userstat[i].num_heuristic_evaluations_bluestar[k] );
			}


			// Scrivo su file
			myfile << endl << "MEDIE" << endl;
			myfile << "Prefix: " << userstat[i].prefix << endl;
			myfile << "Num states B: " << boost::accumulators::mean(acc[0]) << endl;
			myfile << "Percentage positive B: " << boost::accumulators::mean(acc[1])  << endl;
			myfile << "Errore Rate B: " << boost::accumulators::mean(acc[2]) << endl;
			myfile << "Num actual merges B: " << boost::accumulators::mean(acc[3]) << endl;
			myfile << "Num heuristic evaluations B: " << boost::accumulators::mean(acc[4])  << endl;


			myfile << endl << "DEV STD" << endl;
			myfile << "Num states B: " << sqrt(boost::accumulators::variance(acc[0]))  << endl;
			myfile << "Percentage positive B: " << sqrt(boost::accumulators::variance(acc[1]))<< endl;
			myfile << "Errore Rate B: " << sqrt(boost::accumulators::variance(acc[2])) << endl;
			myfile << "Num actual merges B: " << sqrt(boost::accumulators::variance(acc[3])) << endl;
			myfile << "Num heuristic evaluations B: " << sqrt(boost::accumulators::variance(acc[4]))<< endl;


			myfile << endl << "DEV STD NORM " << endl;
			myfile << "Num states B: " << sqrt(boost::accumulators::variance(acc[0]))/boost::accumulators::mean(acc[0])   << endl;
			myfile << "Percentage positive B: " << sqrt(boost::accumulators::variance(acc[1]))/boost::accumulators::mean(acc[1]) << endl;
			myfile << "Errore Rate B: " << sqrt(boost::accumulators::variance(acc[2]))/boost::accumulators::mean(acc[2])  << endl;
			myfile << "Num actual merges B: " << sqrt(boost::accumulators::variance(acc[3]))/boost::accumulators::mean(acc[3])  << endl;
			myfile << "Num heuristic evaluations B: " << sqrt(boost::accumulators::variance(acc[4]))/boost::accumulators::mean(acc[4]) << endl;

		}
		//else
		//	myfile << "-" << endl;
	}

	myfile.close();


	// EDSM GRAFICI
	string graph_title_edsm[4] = {"graph_num_states_edsm", "graph_accuracy_edsm", "graph_actual_merges_edsm", "graph_heuristc_evaluation_edsm"};

	for(int i=0; i<4; ++i)
	{
		// Write in file the positive e negative samples
		ofstream myfile_graph;
		string graph_path = current_exp_folder + graph_title_edsm[i] + ".txt";
		myfile_graph.open(graph_path.c_str(), ios::app);


		// Stampa lunghezza dei prefissi
		for(int j=0; j<n_prefixes; ++j)
			if ( userstat[j].num_heuristic_evaluations_bluestar[0] != -1 ){
				myfile_graph << intTostring(userstat[j].prefix.length()) << " ";
				break;
			}


		// Stampa valore media
		if( i == 0)
			myfile_graph << boost::accumulators::mean(EDSM_value_resulting_from_all_prefixes_together[0]);
		else if( i == 1)
			myfile_graph << boost::accumulators::mean(EDSM_value_resulting_from_all_prefixes_together[1]);
		else if( i == 2)
			myfile_graph << boost::accumulators::mean(EDSM_value_resulting_from_all_prefixes_together[2]);
		else if( i == 3)
			myfile_graph << boost::accumulators::mean(EDSM_value_resulting_from_all_prefixes_together[3]);


		// Stampa dev std
		if( i == 0)
			myfile_graph << "   " << sqrt(boost::accumulators::variance(EDSM_value_resulting_from_all_prefixes_together[0]));
		else if( i == 1)
			myfile_graph << "   " << sqrt(boost::accumulators::variance(EDSM_value_resulting_from_all_prefixes_together[1]));
		else if( i == 2)
			myfile_graph << "   " << sqrt(boost::accumulators::variance(EDSM_value_resulting_from_all_prefixes_together[2]));
		else if( i == 3)
			myfile_graph << "   " << sqrt(boost::accumulators::variance(EDSM_value_resulting_from_all_prefixes_together[3]));



		// Stampa dev std normalizzata
		if( i == 0)
			myfile_graph << "   " << sqrt(boost::accumulators::variance(EDSM_value_resulting_from_all_prefixes_together[0]))/boost::accumulators::mean(EDSM_value_resulting_from_all_prefixes_together[0]) << endl;
		else if( i == 1)
			myfile_graph << "   " << sqrt(boost::accumulators::variance(EDSM_value_resulting_from_all_prefixes_together[1]))/boost::accumulators::mean(EDSM_value_resulting_from_all_prefixes_together[1])  << endl;
		else if( i == 2)
			myfile_graph << "   " << sqrt(boost::accumulators::variance(EDSM_value_resulting_from_all_prefixes_together[2]))/boost::accumulators::mean(EDSM_value_resulting_from_all_prefixes_together[2]) << endl;
		else if( i == 3)
			myfile_graph << "   " << sqrt(boost::accumulators::variance(EDSM_value_resulting_from_all_prefixes_together[3]))/boost::accumulators::mean(EDSM_value_resulting_from_all_prefixes_together[3]) << endl;


		myfile_graph.close();
	}


	// BLUESTAR GRAFICI
	string graph_title[5] = {"graph_num_states_bluestar", "graph_accuracy_bluestar", "graph_error_rate_bluestar", "graph_actual_merges_bluestar", "graph_heuristc_evaluation_blustar"};

	for(int i=0; i<5; ++i)
	{

		// Write in file the positive e negative samples
		ofstream myfile_graph;
		string graph_path = current_exp_folder + graph_title[i] + ".txt";
		myfile_graph.open(graph_path.c_str(), ios::app);


		// Stampa lunghezza dei prefissi
		for(int j=0; j<n_prefixes; ++j)
			if ( userstat[j].num_heuristic_evaluations_bluestar[0] != -1 ){
				myfile_graph << intTostring(userstat[j].prefix.length()) << " ";
				break;
			}


		// Stampa valore media
		if( i == 0)
			myfile_graph << boost::accumulators::mean(value_resulting_from_all_prefixes_together[0]);
		else if( i == 1)
			myfile_graph << boost::accumulators::mean(value_resulting_from_all_prefixes_together[1]);
		else if( i == 2)
			myfile_graph << boost::accumulators::mean(value_resulting_from_all_prefixes_together[2]);
		else if( i == 3)
			myfile_graph << boost::accumulators::mean(value_resulting_from_all_prefixes_together[3]);
		else if( i == 4)
			myfile_graph << boost::accumulators::mean(value_resulting_from_all_prefixes_together[4]);


		// Stampa dev std
		if( i == 0)
			myfile_graph << "   " << sqrt(boost::accumulators::variance(value_resulting_from_all_prefixes_together[0]));
		else if( i == 1)
			myfile_graph << "   " << sqrt(boost::accumulators::variance(value_resulting_from_all_prefixes_together[1]));
		else if( i == 2)
			myfile_graph << "   " << sqrt(boost::accumulators::variance(value_resulting_from_all_prefixes_together[2]));
		else if( i == 3)
			myfile_graph << "   " << sqrt(boost::accumulators::variance(value_resulting_from_all_prefixes_together[3]));
		else if( i == 4)
			myfile_graph << "   " << sqrt(boost::accumulators::variance(value_resulting_from_all_prefixes_together[4]));



		// Stampa dev std normalizzata
		if( i == 0)
			myfile_graph << "   " << sqrt(boost::accumulators::variance(value_resulting_from_all_prefixes_together[0]))/boost::accumulators::mean(value_resulting_from_all_prefixes_together[0]) << endl;
		else if( i == 1)
			myfile_graph << "   " << sqrt(boost::accumulators::variance(value_resulting_from_all_prefixes_together[1]))/boost::accumulators::mean(value_resulting_from_all_prefixes_together[1])  << endl;
		else if( i == 2)
			myfile_graph << "   " << sqrt(boost::accumulators::variance(value_resulting_from_all_prefixes_together[2]))/boost::accumulators::mean(value_resulting_from_all_prefixes_together[2]) << endl;
		else if( i == 3)
			myfile_graph << "   " << sqrt(boost::accumulators::variance(value_resulting_from_all_prefixes_together[3]))/boost::accumulators::mean(value_resulting_from_all_prefixes_together[3]) << endl;
		else if( i == 4)
			myfile_graph << "   " << sqrt(boost::accumulators::variance(value_resulting_from_all_prefixes_together[4]))/boost::accumulators::mean(value_resulting_from_all_prefixes_together[4])  << endl;


		myfile_graph.close();
	}

}



void geoExp::stat_final_results_minimal(mystat* userstat, int n_prefixes, string utente, bool is_edsm)
{
	string file_path_BLUESTAR = "";


	if(cross_val_run == 0){
		cerr << "Cross Validation not specificated" << endl;
		return;
	}


	file_path_BLUESTAR = current_exp_folder + "STAT_MINIMAL_FINAL_RESULTS_EDSM.txt";

	string file_path_EDSM = current_exp_folder+ "STAT_MINIMAL_FINAL_RESULTS_BLUESTAR.txt";

	cout << "Statistics file: "<<file_path_BLUESTAR << endl;


	// Write in file the positive e negative samples
	ofstream myfile;
	myfile.open(file_path_BLUESTAR.c_str(), ios::app);

	myfile << endl << endl << "Utente: "<< utente << " - Lunghezza prefisso: "<< intTostring(userstat[0].prefix.length()) << endl;
	myfile << "Media e varianza del Cross Validation:" << endl;

	for(int i=0; i<n_prefixes; ++i)
	{
		if(is_edsm){

		}
		else if ( userstat[i].num_heuristic_evaluations_bluestar[0] != -1 )
		{
			// Accumulatore per i dati
			boost::accumulators::accumulator_set<double, boost::accumulators::features<boost::accumulators::tag::mean, boost::accumulators::tag::variance> > acc[5];


			// Carico i file nell'accumulators
			for(int k=0; k<cross_val_run; ++k){
				acc[0] ( userstat[i].num_states_bluestar[k] );
				acc[1] ( userstat[i].percentage_positive_bluestar[k] );
				acc[2] ( userstat[i].errore_rate_bluestar[k] );
				acc[3] ( userstat[i].num_actual_merges_bluestar[k] );
				acc[4] ( userstat[i].num_heuristic_evaluations_bluestar[k] );
			}


			// Scrivo su file
			myfile << endl << "MEDIE" << endl;
			myfile << "Prefix: " << userstat[0].prefix << endl;
			myfile << boost::accumulators::mean(acc[0]) << endl;
			myfile << boost::accumulators::mean(acc[1])  << endl;
			myfile << boost::accumulators::mean(acc[2]) << endl;
			myfile << boost::accumulators::mean(acc[3]) << endl;
			myfile << boost::accumulators::mean(acc[4])  << endl;


			myfile << endl << "DEV STD" << endl;
			myfile <<  sqrt(boost::accumulators::variance(acc[0]))  << endl;
			myfile << sqrt(boost::accumulators::variance(acc[1]))<< endl;
			myfile <<  sqrt(boost::accumulators::variance(acc[2])) << endl;
			myfile <<  sqrt(boost::accumulators::variance(acc[3])) << endl;
			myfile <<  sqrt(boost::accumulators::variance(acc[4]))<< endl;


			myfile << endl << "DEV STD NORM " << endl;
			myfile <<  sqrt(boost::accumulators::variance(acc[0]))/boost::accumulators::mean(acc[0])   << endl;
			myfile <<  sqrt(boost::accumulators::variance(acc[1]))/boost::accumulators::mean(acc[1]) << endl;
			myfile << sqrt(boost::accumulators::variance(acc[2]))/boost::accumulators::mean(acc[2])  << endl;
			myfile << sqrt(boost::accumulators::variance(acc[3]))/boost::accumulators::mean(acc[3])  << endl;
			myfile << sqrt(boost::accumulators::variance(acc[4]))/boost::accumulators::mean(acc[4]) << endl;

		}
		//else
		//	myfile << "-" << endl;
	}

	myfile.close();
}




void geoExp::print_execution_parent_folder()
{
	cout<<"Execution dir: "<< exec_path <<endl;
}

void geoExp::print_experiments_folder()
{
	cout<<"Experiments dir: "<< root_exp_path <<endl;
}


void geoExp::get_num_trajectories_for_pref_length(int prefixes_length)
{
	// Utenti analizzati
	string users1[USERS_IN_DB];
	for(int i=0; i<USERS_IN_DB; ++i){
		if(i <10)
			users1[i] = "00"+intTostring(i);
		else if(i>=10 && i<100)
			users1[i] = "0"+intTostring(i);
		else if(i>=100)
			users1[i] = intTostring(i);
	}



	// Open connection to DB
	geodb* mydb = new geodb(db_path);
	mydb->connect();


	// IL numero viene automaticamente stampato dalla funzione del DB!
	for(int i=0; i<USERS_IN_DB; ++i)
		mydb->get_num_minitraj_for_user(users1[i], prefixes_length);



	mydb->close();
	delete mydb;

}



vector<string> geoExp::get_userIDs_as_strings()
{
	vector<string> users;
	// Utenti analizzati
	for(int i=0; i<USERS_IN_DB; ++i){
		if(i <10)
			users.push_back( "00"+intTostring(i));
		else if(i>=10 && i<100)
			users.push_back("0"+intTostring(i));
		else if(i>=100)
			users.push_back(intTostring(i));
	}

	return users;
}

