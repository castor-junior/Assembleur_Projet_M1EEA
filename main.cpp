
#include "Assembleur.h"

int main()
{
    CAssembleur assembleur;

    cout << "==============================" << endl;
    cout << "   MICROCONTROLEUR 8 BITS" << endl;
    cout << "       ASSEMBLEUR" << endl;
    cout << "==============================" << endl;

    string chemin;

    cout << "\nChemin du fichier .asm : ";
    cin >> chemin;

    bool resultat = assembleur.assemble(chemin);

    if (resultat)
    {
        cout << "\nAssemblage reussi !" << endl;

        assembleur.afficheCodeMachine();

        cout << "\nNombre d'octets generes : "
            << assembleur.getCodeMachine().size()
            << endl;

        cout << "\nTable des etiquettes :\n";

        for (const auto& etiquette :
            assembleur.getTabAdresse())
        {
            cout << etiquette.first
                << " = 0x"
                << hex << etiquette.second
                << dec << endl;
        }
    }
    else
    {
        cout << "\nEchec de l'assemblage." << endl;

        assembleur.afficheErreurs();
    }

    return 0;
}
