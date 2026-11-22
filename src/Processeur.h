#pragma once
// ============================================================================
//  Processeur.h  -  Partie A : coeur du microcontroleur 8 bits
//  Aucune dependance a l'interface graphique : seulement IBus.
// ============================================================================
#include <cstdint>
#include <string>
#include "IBus.h"

// Opcodes : IDENTIQUES a ceux de CAssembleur::initOpcodes()  (+ RETI = 0x10)
enum Opcode : uint8_t
{
    OP_NOP  = 0x00,
    OP_LDI  = 0x01,
    OP_LD   = 0x02,
    OP_ST   = 0x03,
    OP_MOV  = 0x04,
    OP_ADD  = 0x05,
    OP_SUB  = 0x06,
    OP_AND  = 0x07,
    OP_OR   = 0x08,
    OP_CMP  = 0x09,
    OP_JMP  = 0x0A,
    OP_JZ   = 0x0B,
    OP_JNZ  = 0x0C,
    OP_CALL = 0x0D,
    OP_RET  = 0x0E,
    OP_HALT = 0x0F,
    OP_RETI = 0x10      // a ajouter dans l'assembleur
};

enum class EtatCPU { EN_MARCHE, ARRETE, ERREUR };

class CProcesseur
{
public:
    static constexpr uint8_t  TAILLE_INSTRUCTION = 3;     // [opcode][op1][op2]
    static constexpr uint8_t  VECTEUR_IT         = 0x03;  // adresse du handler
    static constexpr uint8_t  PILE_INIT          = 0xFF;  // SP au reset
    static constexpr uint8_t  PILE_MIN           = 0x84;  // limite basse de la pile
    static constexpr unsigned CYCLES_ENTREE_IT   = 5;     // cout d'une prise d'interruption

    explicit CProcesseur(IBus& bus);

    // Remise a zero du CPU (registres, PC, SP, indicateurs, cycles).
    // NE remet PAS a zero la memoire ni les peripheriques (c'est B / l'IHM).
    void Reset();

    // Execute UNE instruction (fetch, decode, execute), transmet les cycles
    // au bus, puis traite une eventuelle interruption.
    // Retourne true si une instruction a ete executee,
    // false si le CPU est arrete (HALT) ou en erreur.
    bool Step();

    // ---- Lecture de l'etat (pour l'interface graphique, sans effet de bord)
    uint8_t  Reg(int i) const       { return m_R[i & 7]; }
    uint8_t  PC() const             { return m_PC; }
    uint8_t  SP() const             { return m_SP; }
    bool     Z() const              { return m_Z; }
    bool     C() const              { return m_C; }
    bool     N() const              { return m_N; }
    bool     IE() const             { return m_IE; }
    uint64_t Cycles() const         { return m_cycles; }
    EtatCPU  Etat() const           { return m_etat; }
    bool     EstArrete() const      { return m_etat == EtatCPU::ARRETE; }
    const std::string& MessageErreur() const { return m_erreur; }

    // Registre d'etat sur un octet : bit0 = Z, bit1 = C, bit2 = N
    uint8_t  Indicateurs() const;

    // Utilitaires (affichage / tests)
    static unsigned    CyclesInstruction(uint8_t opcode);
    static const char* NomInstruction(uint8_t opcode);

private:
    IBus&    m_bus;
    uint8_t  m_R[8];
    uint8_t  m_PC;
    uint8_t  m_SP;
    bool     m_Z, m_C, m_N;
    bool     m_IE;            // interruptions autorisees
    uint64_t m_cycles;
    EtatCPU  m_etat;
    std::string m_erreur;

    void Erreur(const std::string& msg);
    bool RegOk(uint8_t n);
    void MajZN(uint8_t resultat);
    bool Empiler(uint8_t valeur);
    bool Depiler(uint8_t& valeur);
    void SetIndicateurs(uint8_t octet);
    void PrendreInterruption();
};
