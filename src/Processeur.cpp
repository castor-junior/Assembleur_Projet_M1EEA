#include "Processeur.h"
#include <cstdio>

CProcesseur::CProcesseur(IBus& bus) : m_bus(bus)
{
    Reset();
}

void CProcesseur::Reset()
{
    for (auto& r : m_R) r = 0;
    m_PC = 0;
    m_SP = PILE_INIT;
    m_Z = m_C = m_N = false;
    m_IE = true;
    m_cycles = 0;
    m_etat = EtatCPU::EN_MARCHE;
    m_erreur.clear();
}

uint8_t CProcesseur::Indicateurs() const
{
    return (m_Z ? (1 << 0) : 0) |
           (m_C ? (1 << 1) : 0) |
           (m_N ? (1 << 2) : 0);
}

void CProcesseur::SetIndicateurs(uint8_t o)
{
    m_Z = (o & (1 << 0)) != 0;
    m_C = (o & (1 << 1)) != 0;
    m_N = (o & (1 << 2)) != 0;
}

void CProcesseur::Erreur(const std::string& msg)
{
    m_etat = EtatCPU::ERREUR;
    m_erreur = msg;
}

bool CProcesseur::RegOk(uint8_t n)
{
    if (n > 7)
    {
        Erreur("Registre invalide (R" + std::to_string(n) + ") a l'adresse "
               + std::to_string(m_PC));
        return false;
    }
    return true;
}

void CProcesseur::MajZN(uint8_t r)
{
    m_Z = (r == 0);
    m_N = (r & 0x80) != 0;
}

// Pile en RAM, descend depuis 0xFF : ecrire a SP puis SP--
bool CProcesseur::Empiler(uint8_t v)
{
    if (m_SP < PILE_MIN)
    {
        Erreur("Debordement de pile");
        return false;
    }
    m_bus.Ecrire(m_SP, v);
    m_SP--;
    return true;
}

// SP++ puis lire a SP
bool CProcesseur::Depiler(uint8_t& v)
{
    if (m_SP == PILE_INIT)
    {
        Erreur("Pile vide (RET/RETI sans CALL)");
        return false;
    }
    m_SP++;
    v = m_bus.Lire(m_SP);
    return true;
}

unsigned CProcesseur::CyclesInstruction(uint8_t op)
{
    switch (op)
    {
    case OP_NOP:  return 1;
    case OP_LDI:  return 2;
    case OP_LD:   return 3;
    case OP_ST:   return 3;
    case OP_MOV:  return 1;
    case OP_ADD: case OP_SUB: case OP_AND: case OP_OR: case OP_CMP: return 2;
    case OP_JMP: case OP_JZ: case OP_JNZ: return 2;
    case OP_CALL: return 4;
    case OP_RET:  return 3;
    case OP_RETI: return 4;
    case OP_HALT: return 1;
    default:      return 0;
    }
}

const char* CProcesseur::NomInstruction(uint8_t op)
{
    switch (op)
    {
    case OP_NOP:  return "NOP";
    case OP_LDI:  return "LDI";
    case OP_LD:   return "LD";
    case OP_ST:   return "ST";
    case OP_MOV:  return "MOV";
    case OP_ADD:  return "ADD";
    case OP_SUB:  return "SUB";
    case OP_AND:  return "AND";
    case OP_OR:   return "OR";
    case OP_CMP:  return "CMP";
    case OP_JMP:  return "JMP";
    case OP_JZ:   return "JZ";
    case OP_JNZ:  return "JNZ";
    case OP_CALL: return "CALL";
    case OP_RET:  return "RET";
    case OP_RETI: return "RETI";
    case OP_HALT: return "HALT";
    default:      return "???";
    }
}

bool CProcesseur::Step()
{
    if (m_etat != EtatCPU::EN_MARCHE)
        return false;

    // ---- FETCH : 3 octets [opcode][op1][op2]
    const uint8_t op = m_bus.Lire(m_PC);
    const uint8_t a  = m_bus.Lire(static_cast<uint8_t>(m_PC + 1));
    const uint8_t b  = m_bus.Lire(static_cast<uint8_t>(m_PC + 2));

    const unsigned cyc = CyclesInstruction(op);
    if (cyc == 0)
    {
        char tampon[64];
        snprintf(tampon, sizeof(tampon), "Opcode inconnu 0x%02X a l'adresse 0x%02X", op, m_PC);
        Erreur(tampon);
        return false;
    }

    uint8_t pcSuivant = static_cast<uint8_t>(m_PC + TAILLE_INSTRUCTION);

    // ---- DECODE + EXECUTE
    switch (op)
    {
    case OP_NOP:
        break;

    case OP_LDI:                                   // Rd <- imm
        if (!RegOk(a)) return false;
        m_R[a] = b;
        break;

    case OP_LD:                                    // Rd <- mem[addr]
        if (!RegOk(a)) return false;
        m_R[a] = m_bus.Lire(b);
        break;

    case OP_ST:                                    // mem[addr] <- Rs
        if (!RegOk(a)) return false;
        m_bus.Ecrire(b, m_R[a]);
        break;

    case OP_MOV:                                   // Rd <- Rs
        if (!RegOk(a) || !RegOk(b)) return false;
        m_R[a] = m_R[b];
        break;

    case OP_ADD:
    {
        if (!RegOk(a) || !RegOk(b)) return false;
        const uint16_t r = static_cast<uint16_t>(m_R[a]) + m_R[b];
        m_C = (r > 0xFF);
        m_R[a] = static_cast<uint8_t>(r);
        MajZN(m_R[a]);
        break;
    }

    case OP_SUB:
    case OP_CMP:                                   // CMP = SUB sans ecriture
    {
        if (!RegOk(a) || !RegOk(b)) return false;
        const uint8_t r = static_cast<uint8_t>(m_R[a] - m_R[b]);
        m_C = (m_R[a] < m_R[b]);                   // C = emprunt
        if (op == OP_SUB) m_R[a] = r;
        MajZN(r);
        break;
    }

    case OP_AND:
        if (!RegOk(a) || !RegOk(b)) return false;
        m_R[a] &= m_R[b];
        m_C = false;
        MajZN(m_R[a]);
        break;

    case OP_OR:
        if (!RegOk(a) || !RegOk(b)) return false;
        m_R[a] |= m_R[b];
        m_C = false;
        MajZN(m_R[a]);
        break;

    case OP_JMP:
        pcSuivant = a;
        break;

    case OP_JZ:
        if (m_Z) pcSuivant = a;
        break;

    case OP_JNZ:
        if (!m_Z) pcSuivant = a;
        break;

    case OP_CALL:                                  // empile l'adresse de retour
        if (!Empiler(pcSuivant)) return false;
        pcSuivant = a;
        break;

    case OP_RET:
        if (!Depiler(pcSuivant)) return false;
        break;

    case OP_RETI:                                  // depile indicateurs puis PC
    {
        uint8_t flags = 0;
        if (!Depiler(flags) || !Depiler(pcSuivant)) return false;
        SetIndicateurs(flags);
        m_IE = true;
        break;
    }

    case OP_HALT:
        m_etat = EtatCPU::ARRETE;
        pcSuivant = m_PC;                          // PC reste sur le HALT
        break;
    }

    m_PC = pcSuivant;
    m_cycles += cyc;

    // ---- CADENCEMENT : les peripheriques avancent du meme nombre de cycles
    m_bus.Tick(cyc);

    // ---- INTERRUPTION : uniquement ENTRE deux instructions
    if (m_etat == EtatCPU::EN_MARCHE && m_IE && m_bus.InterruptionEnAttente())
        PrendreInterruption();

    return true;
}

// Sauvegarde PC puis indicateurs, masque les interruptions, saute au vecteur.
void CProcesseur::PrendreInterruption()
{
    if (!Empiler(m_PC) || !Empiler(Indicateurs()))
        return;                                    // etat = ERREUR (pile)

    m_IE = false;
    m_bus.AcquitterInterruption();
    m_PC = VECTEUR_IT;
    m_cycles += CYCLES_ENTREE_IT;
    m_bus.Tick(CYCLES_ENTREE_IT);
}
