#pragma once
// ============================================================================
//  IBus.h  -  CONTRAT entre le processeur  et memoire/peripheriques
//  .  A ne connait QUE cette interface.  B fournit CBus : public IBus.
//
//  Carte memoire (espace d'adressage 8 bits, 256 octets) :
//    0x00-0x7F : code et donnees du programme
//    0x80      : port de sortie (8 LED)
//    0x81      : port d'entree (8 interrupteurs)
//    0x82      : timer, valeur N (periode en cycles)
//    0x83      : timer, controle (bit 0 = activer)
//    0x84-0xFF : RAM, la pile descend depuis 0xFF
// ============================================================================
#include <cstdint>

class IBus
{
public:
    virtual ~IBus() = default;

    // Acces du CPU (peuvent avoir des effets de bord sur les peripheriques)
    virtual uint8_t Lire(uint8_t adresse) = 0;
    virtual void    Ecrire(uint8_t adresse, uint8_t valeur) = 0;

    // Lecture SANS effet de bord (dump memoire de l'interface graphique)
    virtual uint8_t Observer(uint8_t adresse) const = 0;

    // Appele par le CPU apres CHAQUE instruction avec le nombre de cycles
    // consommes : les peripheriques (timer) avancent d'autant.
    virtual void Tick(unsigned cycles) = 0;

    // Une interruption est-elle demandee par un peripherique ?
    virtual bool InterruptionEnAttente() const = 0;

    // Le CPU appelle ceci au moment ou il PREND l'interruption :
    // le peripherique doit baisser sa ligne de demande.
    virtual void AcquitterInterruption() = 0;
};
