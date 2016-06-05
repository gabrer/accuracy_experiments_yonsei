/*
 * edsm.h
 */

#ifndef GEOEXP_H_
#define GEOEXP_H_

#include "edsm.h"
#include "messages.h"
#include <boost/filesystem.hpp>
#include "lstar.h"
#include "blueStar.h"
#include "utilities.h"
#include "geodb.h"
#include <unordered_map>

#include <boost/accumulators/accumulators.hpp>	// For Accumulators
#include <boost/accumulators/statistics.hpp>			// To calculate mean and variance for an accumulator

#include <math.h>

#include "bluefringe.h"

#define MAX_LENGTH_SAMPLES_POS 	1000
#define MAX_LENGTH_SAMPLES_NEG 	1000


using namespace std;


class geoExp
{
private:

	string 		db_path;

	string 		exec_path;													/*!< The project folder, parent of execution directory */
	string		root_exp_path;												/*!< Dir with all experiments: "experiments" */
	string		current_exp_folder;											/*!< Folder for current experiment, charartterizated by user */

	int 		user;
	int 		min_prefixes_length;										/*!< Length of geohash strings */
	int 		max_prefixes_length;										/*!< Length of geohash strings */

	int 		cross_val_run = -1;											/*!< Number of run for cross-validation */
	//int 		num_of_random_sets = -1;										/*!< Num of random sets when CrosVal is not used! */

	bool 		edsm;

	bool		blueStar;
	double	 	alpha_value;												/*!< Alpha value for blue star */
	double 		delta_value;												/*!< Parameter for Z-value. Final evaluations should be indipendent from that particular value \
	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 for numeric issues is better if it is greater than 1000 */

	int			training_proportion;										/*!< Training proportion of strings in percentage */
	int			test_proportion;											/*!< Test proportion of strings in percentage */


	bool		no_repetitions_inside_strings;								/*!< If true delete the repetitions of symbols inside strings; substituted with one occurence*/

	string 		geo_alph =													/*!< Geohash alphabet symbols */
					  { '0', '1', '2', '3', '4', '5', '6', '7',
						'8', '9', 'b', 'c', 'd', 'e', 'f', 'g',
						'h', 'j', 'k', 'm', 'n', 'p', 'q', 'r',
						's', 't', 'u', 'v', 'w', 'x', 'y', 'z' };



	// Structure for statistics
	struct mystat {
		string 		prefix = "";																// Identify the inference process
		int 		num_states_edsm 	  						= -1;
		int*		num_states_bluestar;
		double 		percentage_positive_edsm				= -1;				// Cicla le LUNGHEZZE dei prefissi (for statistical purpose... see below)
		double* 	percentage_positive_bluestar;
		double* 	errore_rate_bluestar;												// Error rate on the training dataset
		int			num_actual_merges_edsm 			= -1;
		int			num_heuristic_evaluations_edsm 	= -1;
		int*		num_actual_merges_bluestar;
		int*		num_heuristic_evaluations_bluestar ;
	};


	void 		set_root_exp_folders();

	void 		set_current_exp_folder();

	string 		create_folder(const string  base_path, const string new_folder, bool current_time);

	vector<string> 	get_userIDs_as_strings();

	int 		write_minitraj_from_db_like_samples_TRAINTEST_TESTSET(string user,string prefix, string path_samples);

	void 		write_minitraj_from_db_like_samples_RANDOM_REDUCED_SAMPLE(string user,string prefix, string path_samples);

	void 		write_minitrajectories_as_training_set(vector<string>* p_samples, vector<string>* n_samples, const char * file_path);

	void 		write_minitrajectories_as_test_set(vector<string>* test_samples, const char * file_path);


	string* 	read_testsamples_leaving_geohash_encoding(const char * /*path*/ path_samples, int &dim_positive, int* &wp);

	double 		make_generalization_test(gi::dfa* finaldfa, const char * file_path/*, int dim_test*/);

	string		add_space_between_characters_delete_repetitions(string old_string);





	////////////////////////////////////////////////////////
	// STATISTICS

	void		print_execution_parent_folder();

	void 		print_experiments_folder();


	void 		write_statistics_files(mystat* userstat, int n_prefixes, string utente, bool is_edsm, int curr_run_cross_val);

	void 		stat_write_generalization_rate(mystat* userstat, int n_prefixes, string utente, bool is_edsm, int curr_run_cross_val);

	void 		stat_write_num_states(mystat* userstat, int n_prefixes, string utente, bool is_edsm, int curr_run_cross_val);

	void 		stat_write_error_rate_on_trainigset(mystat* userstat, int n_prefixes, string utente, bool is_edsm, int curr_run_cross_val);

	void 		stat_heuristic_evaluations(mystat* userstat, int n_prefixes, string utente, bool is_edsm, int curr_run_cross_val);

	void 		stat_actual_merges(mystat* userstat, int n_prefixes, string utente, bool is_edsm, int curr_run_cross_val);

	void 		stat_final_results(mystat* userstat, int n_prefixes, string utente, bool is_edsm);

	void 		stat_final_results_minimal(mystat* userstat, int n_prefixes, string utente, bool is_edsm);


	void 		get_num_trajectories_for_pref_length(int prefixes_length);


public:

	/**
	 * Instance an object with all the members and methods for EDSM inference process.
	 * @param path It's the path where find positive and negative samples
	 */
	geoExp(string db_path, int user, int min_prefixes_length, int max_prefixes_length, bool repetitions, int train_proportion,\
			 bool alg_edsm, bool alg_blues, double alpha, double delta);



	~geoExp();


	void 		run_inference_accuracy();


};


#endif /* EDSM_H_ */
