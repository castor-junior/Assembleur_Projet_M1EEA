#include <iostream>
#include "CAssembleur.h"

// Constructeur
CAssembleur::CAssembleur() { initOpcodes(); }

// Initialisation des opcodes
void CAssembleur::initOpcodes()
{
	// opcodes a determiner, ici c'est des exemples
	opcodes = {
		{"NOP", 0x00},
		{"LDI", 0x01},
		{"LD", 0x02},
		{"ST", 0x03},
		{"MOV", 0x04},
		{"ADD", 0x05},
		{"SUB", 0x06},
		{"AND", 0x07},
		{"OR", 0x08},
		{"CMP", 0x09},
		{"JMP", 0x0A},
		{"JZ", 0x0B},
		{"JNZ", 0x0C},
		{"CALL", 0x0D},
		{"RET", 0x0E},
		{"HALT", 0x0F},
        {"RETI", 0x10}
	};
}

// Conversion en majuscules
string CAssembleur::Maj(string txt)
{
	
	transform(txt.begin(), txt.end(), txt.begin(), ::toupper);  //prends txt, passe en majuscule, met le resultat dans txt.

	return txt;
}

// Nettoyage d'une ligne, enleve commentaire et "   LDR R0, R2  " ==>  "LDR R0, R2"
string CAssembleur::cleanLigne(string ligne)
{
	// suppression des commentaires
	size_t position = ligne.find(';');      // trouve la premiere occurence de ';' et met ca position dans position
	
	if (position != string::npos)           // si pas de correspondance, position == string::npos, -1. si correspondance, entrer dans le if
		ligne = ligne.substr(0, position);  // met dans la varialbe 'ligne' tout ce qu'il y'a à partir de la position 0 et de longueur 'position'

	// suppression des espaces en recherchant les caractères utiles
	size_t debut = ligne.find_first_not_of(" \t\r\n");      // chercher la premiere occurence de symbole qui ne correspond PAS a " \t\r\n"
                                                            // pour espace, tabulation, retour chariot, saut de ligne
	if (debut == string::npos) return "";                   // cas d'une ligne vide

	size_t fin = ligne.find_last_not_of(" \t\r\n");         // trouve le dernier caractère utile
	
	ligne = ligne.substr(debut, fin - debut + 1);           

	return ligne;
}

// Conversion d'une valeur numerique de la forme char à int (string to integer)
int CAssembleur::convertirNb(string txt)
{
    txt = cleanLigne(txt);

    size_t position = 0;

    int valeur = stoi(txt, &position, 0);       // stoi = string to interger

    if (position != txt.size())
        throw invalid_argument("Nombre invalide");

    if (valeur < 0 || valeur > 255)
        throw out_of_range("Valeur hors limites (0 a 255)");

    return valeur;
}

// Verification de la syntaxe d'un registre et conversion du registre en numero désignant le registre
int CAssembleur::convertirReg(string texte)  
{
    texte = Maj(texte);     // Convertir en majuscule

    if (texte.size() != 2 || texte[0] != 'R')       // Verification de la conversion en majuscule et qu'il n'y a que deux caractères
        throw invalid_argument("Registre invalide");

    int numero = texte[1] - '0';                    // Convertir le deuxième caractère en nombre

    if (numero < 0 || numero > 7)                   // Lit le nombre pour vérifier qu'on n'utilise pas un registre inexistant
        throw out_of_range("Registre inexistant");

    return numero;
}

// Ajout d'une erreur
void CAssembleur::ajoutErreur(int ligne, string message)
{
    erreurs.push_back({ ligne, message });
}

// Analyse d'une ligne et cree une SInstruction pour la représenter
bool CAssembleur::analyseLigne(string ligne, int numeroLigne)
{
    ligne = cleanLigne(ligne);      // enleve les commentaires et espace en début et fin de ligne

    if (ligne.empty())
        return true;

    // Detection d'une etiquette
    size_t position = ligne.find(':');

    if (position != string::npos)       // SI une etiquette a ete detecte
    {
        string etiquette = ligne.substr(0, position);       // Recuperer l'etiquette

        if (etiquette.empty())
        {
            ajoutErreur(numeroLigne, "Etiquette vide");     // Verification de la recuperation
            return false;
        }

        if (tabAdresses.count(etiquette))                   // Verifier que l'etiquette n'a pas deja ete cree avant
        {
            ajoutErreur(numeroLigne, "Etiquette deja definie");
            return false;
        }

        tabAdresses[etiquette] = instructions.size() * tailleInstruction;       // enregistre l'addresse de l'etiquette. la multiplication donne la prochaine adresse a utiliser

        ligne = cleanLigne(ligne.substr(position + 1));              // mettre dans "ligne" ce qu'il y'a après les ":"           

        if (ligne.empty())                                          // rien après ":", donc la ligne n'a qu'une etiquette, fin
            return true;
    }

    // Extraction du nom de l'instruction
    stringstream flux(ligne);

    string nomInstruction;

    flux >> nomInstruction;         // extrait le premier element du flux

    nomInstruction = Maj(nomInstruction);

    if (opcodes.find(nomInstruction) == opcodes.end())      // si find() renvoie end(), l'instruction n'existe pas
    {
        ajoutErreur(numeroLigne, "Instruction inconnue");
        return false;
    }

    // Extraction des operandes
    string reste;

    getline(flux, reste);       // recuperer tout ce qu'il y'a après l'instruction

    vector<string> operandes;

    string operande;

    stringstream fluxOperandes(reste);

    while (getline(fluxOperandes, operande, ','))
    {
        operande = cleanLigne(operande);

        if (!operande.empty())
            operandes.push_back(operande);
    }

    // Nombre d'operandes attendu
    int nombreAttendu = 0;

    if (nomInstruction == "LDI" ||
        nomInstruction == "LD" ||
        nomInstruction == "ST" ||
        nomInstruction == "MOV" ||
        nomInstruction == "ADD" ||
        nomInstruction == "SUB" ||
        nomInstruction == "AND" ||
        nomInstruction == "OR" ||
        nomInstruction == "CMP")
    {
        nombreAttendu = 2;
    }

    if (nomInstruction == "JMP" ||
        nomInstruction == "JZ" ||
        nomInstruction == "JNZ" ||
        nomInstruction == "CALL")
    {
        nombreAttendu = 1;
    }

    if ((int)operandes.size() != nombreAttendu)
    {
        ajoutErreur(numeroLigne,
            "Nombre d'operandes incorrect");
        return false;
    }

    SInstruction instruction;

    instruction.opcode = nomInstruction;
    instruction.operandes = operandes;
    instruction.nrLigne = numeroLigne;
    instruction.adresse = instructions.size() * tailleInstruction;      

    instructions.push_back(instruction);

    return true;
}

// Premiere passe
bool CAssembleur::premierePasse(
    const vector<string>& lignes)
{
    instructions.clear();
    tabAdresses.clear();

    for (size_t i = 0; i < lignes.size(); i++)
    {
        analyseLigne(lignes[i], i + 1);
    }

    return erreurs.empty();
}

// Deuxieme passe
bool CAssembleur::deuxiemePasse()
{
    codeMachine.clear();
    infos.clear();

    for (const auto& instruction : instructions)
    {
        try
        {
            uint8_t opcode = opcodes.at(instruction.opcode);

            uint8_t op1 = 0;
            uint8_t op2 = 0;

            string nom = instruction.opcode;

            if (nom == "LDI")
            {
                op1 = convertirReg(
                    instruction.operandes[0]);

                op2 = convertirNb(
                    instruction.operandes[1]);
            }

            else if (nom == "LD" || nom == "ST")
            {
                op1 = convertirReg(
                    instruction.operandes[0]);

                op2 = convertirNb(
                    instruction.operandes[1]);
            }

            else if (nom == "MOV" || nom == "ADD" ||
                nom == "SUB" || nom == "AND" ||
                nom == "OR" || nom == "CMP")
            {
                op1 = convertirReg(
                    instruction.operandes[0]);

                op2 = convertirReg(
                    instruction.operandes[1]);
            }

            else if (nom == "JMP" || nom == "JZ" ||
                nom == "JNZ" || nom == "CALL")
            {
                string cible = instruction.operandes[0];

                auto it = tabAdresses.find(cible);

                if (it != tabAdresses.end())
                {
                    int adresse = it->second;

                    if (adresse > 255)
                        throw out_of_range(
                            "Adresse superieure a 255");

                    op1 = adresse;
                }
                else
                {
                    op1 = convertirNb(cible);
                }
            }

            // Generation des trois octets
            codeMachine.push_back(opcode);
            codeMachine.push_back(op1);
            codeMachine.push_back(op2);

            // Conservation du lien avec le fichier source
            infos.push_back({
                instruction.adresse,
                instruction.nrLigne,
                instruction.opcode
                });
        }
        catch (const exception& e)
        {
            ajoutErreur(
                instruction.nrLigne,
                e.what()
            );
        }
    }

    return erreurs.empty();
}

// Assemblage complet
bool CAssembleur::assemble(string cheminFichier)
{
    instructions.clear();
    codeMachine.clear();
    erreurs.clear();
    infos.clear();
    tabAdresses.clear();

    ifstream fichier(cheminFichier);

    if (!fichier.is_open())
    {
        ajoutErreur(0, "Impossible d'ouvrir le fichier");
        return false;
    }

    vector<string> lignes;

    string ligne;

    while (getline(fichier, ligne))
    {
        lignes.push_back(ligne);
    }

    fichier.close();

    if (!premierePasse(lignes))
        return false;

    if (!deuxiemePasse())
        return false;

    // Limite de la memoire
    if (codeMachine.size() > 256)
    {
        ajoutErreur(0, "Programme trop volumineux");
        codeMachine.clear();
        return false;
    }

    return true;
}

// Affichage des erreurs
void CAssembleur::afficheErreurs()
{
    for (const auto& erreur : erreurs)
    {
        cout << "Erreur ligne "
            << erreur.ligne << " : "
            << erreur.message << endl;
    }
}

// Affichage du code machine généré en hexa
void CAssembleur::afficheCodeMachine() 
{
    cout << "\nCODE MACHINE\n";

    for (size_t i = 0; i < codeMachine.size(); i++)
    {
        if (i % 3 == 0)
        {
            cout << "\nAdresse 0x"
                << hex << uppercase
                << setw(2) << setfill('0')
                << i << " : ";
        }

        cout << "0x"
            << hex << uppercase
            << setw(2) << setfill('0')
            << (int)codeMachine[i] << " ";
    }

    cout << dec << endl;
}

// Recuperation du code machine
vector<uint8_t>& CAssembleur::getCodeMachine() 
{
    return codeMachine;
}

// Recuperation des informations source
vector<SInfoInstruction>& CAssembleur::getInfo()
{
    return infos;
}

// Recuperation des etiquettes
map<string, int>& CAssembleur::getTabAdresse()
{
    return tabAdresses;
}


