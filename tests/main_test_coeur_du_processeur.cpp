// ============================================================================
//  main_test.cpp - Test console AUTONOME de la partie A (sans partie B ni IHM)
//  Un bus factice (RAM 256 octets + ports + timer simule) implemente IBus.
//  Compilation :  g++ -std=c++17 -Wall -Wextra Processeur.cpp main_test.cpp -o testA
// ============================================================================
#include <iostream>
#include <vector>
#include <cstdlib>
#include "Processeur.h"

using namespace std;

// ---- Bus factice : ce que B remplacera par le vrai CBus -------------------
class CBusTest : public IBus
{
public:
    uint8_t  ram[256] = {0};
    uint8_t  sortie   = 0;
    unsigned periodeTimer = 0;      // 0 = timer desactive
    unsigned acc = 0;
    bool     it = false;

    void Charger(const vector<uint8_t>& code)
    {
        for (size_t i = 0; i < code.size() && i < 0x80; i++) ram[i] = code[i];
    }
    uint8_t Lire(uint8_t a) override           { return Observer(a); }
    void    Ecrire(uint8_t a, uint8_t v) override
    {
        if (a == 0x80) { sortie = v; cout << "   [LED 0x80] = 0x" << hex << (int)v << dec << "\n"; }
        else if (a == 0x81) {}                       // port d'entree : lecture seule
        else ram[a] = v;
    }
    uint8_t Observer(uint8_t a) const override { return (a == 0x81) ? 0x00 : ram[a]; }
    void    Tick(unsigned c) override
    {
        if (!periodeTimer) return;
        acc += c;
        while (acc >= periodeTimer) { acc -= periodeTimer; it = true; }
    }
    bool InterruptionEnAttente() const override { return it; }
    void AcquitterInterruption() override       { it = false; }
};

static int echecs = 0;
#define VERIF(cond, msg) do { if (!(cond)) { cout << "  ECHEC : " << msg << "\n"; echecs++; } \
                              else cout << "  ok    : " << msg << "\n"; } while (0)

static void Lancer(CProcesseur& cpu, int maxInstr = 10000)
{
    while (maxInstr-- > 0 && cpu.Step()) {}
}

int main()
{
    // ------------------------------------------------------------------
    // TEST 1 : le test.txt de l'assembleur (assemble a la main)
    //   LDI R0,5 / LDI R1,1 / boucle: SUB R0,R1 / JNZ boucle / ST R0,0x80 / HALT
    // ------------------------------------------------------------------
    cout << "TEST 1 : programme test.txt\n";
    {
        CBusTest bus;
        bus.Charger({ 0x01,0x00,0x05,  0x01,0x01,0x01,
                      0x06,0x00,0x01,  0x0C,0x06,0x00,
                      0x03,0x00,0x80,  0x0F,0x00,0x00 });
        CProcesseur cpu(bus);
        Lancer(cpu);
        VERIF(cpu.EstArrete(),        "CPU arrete par HALT");
        VERIF(cpu.Reg(0) == 0,        "R0 == 0");
        VERIF(cpu.Reg(1) == 1,        "R1 == 1");
        VERIF(cpu.Z(),                "indicateur Z == 1");
        VERIF(bus.sortie == 0,        "port 0x80 == 0");
        VERIF(cpu.Cycles() == 28,     "28 cycles (obtenu : " + to_string(cpu.Cycles()) + ")");
        VERIF(cpu.PC() == 0x0F,       "PC reste sur le HALT (0x0F)");

        cpu.Reset();
        VERIF(cpu.PC() == 0 && cpu.Cycles() == 0 && cpu.Reg(0) == 0 && !cpu.EstArrete(),
              "Reset remet le CPU a zero");
    }

    // ------------------------------------------------------------------
    // TEST 2 : indicateurs C et N, CALL / RET, AND/OR/MOV
    // ------------------------------------------------------------------
    cout << "TEST 2 : ADD avec retenue, CALL/RET\n";
    {
        CBusTest bus;
        bus.Charger({
            /*00*/ 0x01,0x00,0xFF,      // LDI R0,0xFF
            /*03*/ 0x01,0x01,0x02,      // LDI R1,2
            /*06*/ 0x05,0x00,0x01,      // ADD R0,R1  -> 0x01, C=1
            /*09*/ 0x0D,0x15,0x00,      // CALL sub (0x15)
            /*0C*/ 0x04,0x03,0x02,      // MOV R3,R2
            /*0F*/ 0x0F,0x00,0x00,      // HALT
            /*12*/ 0x00,0x00,0x00,      // NOP (remplissage)
            /*15*/ 0x01,0x02,0x2A,      // sub: LDI R2,42
            /*18*/ 0x0E,0x00,0x00 });   // RET
        CProcesseur cpu(bus);
        Lancer(cpu);
        VERIF(cpu.Reg(0) == 0x01,  "0xFF + 2 = 0x01");
        VERIF(cpu.C(),             "retenue C == 1");
        VERIF(cpu.Reg(3) == 42,    "CALL/RET : R3 == 42");
        VERIF(cpu.SP() == 0xFF,    "pile equilibree (SP == 0xFF)");
    }

    // ------------------------------------------------------------------
    // TEST 3 : interruption timer + RETI (les indicateurs sont preserves)
    //   0x00 JMP main / 0x03 JMP isr / main compte jusqu'a 3 interruptions
    // ------------------------------------------------------------------
    cout << "TEST 3 : interruption + RETI\n";
    {
        CBusTest bus;
        bus.periodeTimer = 20;
        bus.Charger({
            /*00*/ 0x0A,0x09,0x00,      // JMP main
            /*03*/ 0x0A,0x1B,0x00,      // JMP isr  (vecteur)
            /*06*/ 0x00,0x00,0x00,
            /*09*/ 0x01,0x07,0x00,      // main: LDI R7,0
            /*0C*/ 0x01,0x01,0x01,      //       LDI R1,1
            /*0F*/ 0x01,0x02,0x03,      //       LDI R2,3
            /*12*/ 0x09,0x07,0x02,      // loop: CMP R7,R2
            /*15*/ 0x0C,0x12,0x00,      //       JNZ loop
            /*18*/ 0x0F,0x00,0x00,      //       HALT
            /*1B*/ 0x05,0x07,0x01,      // isr:  ADD R7,R1
            /*1E*/ 0x10,0x00,0x00 });   //       RETI
        CProcesseur cpu(bus);
        Lancer(cpu);
        VERIF(cpu.EstArrete(),     "le programme se termine (HALT)");
        VERIF(cpu.Reg(7) >= 3,     "au moins 3 interruptions traitees (R7 = " + to_string(cpu.Reg(7)) + ")");
        VERIF(cpu.IE(),            "interruptions reautorisees apres RETI");
        VERIF(cpu.SP() == 0xFF,    "pile equilibree apres les interruptions");
    }

    // ------------------------------------------------------------------
    // TEST 4 : erreurs (debordement de pile, opcode inconnu)
    // ------------------------------------------------------------------
    cout << "TEST 4 : gestion des erreurs\n";
    {
        CBusTest bus;
        bus.Charger({ 0x0D,0x00,0x00 });           // CALL 0 : recursion infinie
        CProcesseur cpu(bus);
        Lancer(cpu);
        VERIF(cpu.Etat() == EtatCPU::ERREUR, "debordement de pile detecte : " + cpu.MessageErreur());

        CBusTest bus2;
        bus2.Charger({ 0x7E,0x00,0x00 });
        CProcesseur cpu2(bus2);
        Lancer(cpu2);
        VERIF(cpu2.Etat() == EtatCPU::ERREUR, "opcode inconnu detecte : " + cpu2.MessageErreur());
    }

    cout << "\n" << (echecs ? "DES TESTS ONT ECHOUE" : "TOUS LES TESTS PASSENT") << "\n";
    return echecs ? EXIT_FAILURE : EXIT_SUCCESS;
}
