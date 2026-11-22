#pragma once



#include <iostream>		// pour cout, cin, endl
#include <fstream>		// Operation de lecture et ecriture dans les fichiers. ici ouvre et lit .asm avec ifstream
#include <sstream>		// manipuler des string comme des flux
#include <vector>
#include <map>			// utiliser map : pair etiquettes + adresses
#include <string>		
#include <cstdint>		// pour uint8_t ?
#include <algorithm>	// pour transform() dans Maj()
#include <iomanip>		// format d'affichage. hex, uppercase, setw(2), setfill('0')
#include <stdexcept>	// exceptions. invalid_argument, out_of_range, exception pour recuperer des erreurs

using namespace std;

// Un instruction assembleur qui a ete analysee
struct SInstruction
{
	string opcode;				// nom de l'instruction
	vector<string> operandes;	// les arguments de l'instruction
	int nrLigne;				// ligne de l'instruction, utile pour repérer erreurs
	int adresse;				// adresse ou sera place l'instruction
};

// Une erreur d'assemblage
struct SErreur
{
	int ligne;					// Ligne de l'erreur
	string message;				// Message décrivant l'erreur
};

// Information permettant de relier le code machine au fichier source
struct SInfoInstruction
{
	int adresse;
	int nrLigne;
	string txt;
};

// Class de l'assembleur
class CAssembleur
{
	vector<SInstruction> instructions;		// Stocker les instructions analysees
	vector<uint8_t> codeMachine;			// octets generes
	vector<SErreur> erreurs;				// les erreurs detectees
	vector<SInfoInstruction> infos;			// correspondance entre instructions du fichier source et leurs adresse memoire.
	map<string, int> tabAdresses;			// association des etiquettes et leurs adresses la memoire ou dans "instructions" ?
	map<string, uint8_t> opcodes;			// association instructions et leurs code numérique.
	int tailleInstruction = 3;				// Opcode | Operande 1 | Operande 2. Prends donc 3 positions dans le vector instructions

	// Init des opcodes
	void initOpcodes();

	// Nettoyage de texte
	string cleanLigne(string ligne);

	// Conversion en maj
	string Maj(string txt);

	// conversion d'une valeur numerique
	int convertirNb(string txt);

	// conversion d'un registre R0 a R7
	int convertirReg(string txt);

	// analyse d'une ligne
	bool analyseLigne(string ligne, int numeroLigne);

	// Premiere passe : etiquettes
	bool premierePasse(const vector<string>& lignes);

	// Deuxieme passe : code machine
	bool deuxiemePasse();

	// ajouter une erreur
	void ajoutErreur(int ligne, string message);

public:

	CAssembleur();		// constructeur

	// assemblage complet
	bool assemble(string cheminFichier);

	// affichage des erreurs
	void afficheErreurs();

	// affichage du code machine
	void afficheCodeMachine();

	// Recuperer le code machine
	vector<uint8_t>& getCodeMachine();

	// Recuperer les informations source
	vector<SInfoInstruction>& getInfo();

	//recuperer les adresses
	map<string, int>& getTabAdresse();

};





