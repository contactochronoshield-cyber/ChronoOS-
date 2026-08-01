#!/bin/sh
# ==============================================================================
# ChronoOS 2D Entity & DNA Evolution Engine v1.0
# ==============================================================================

DNA_DIR="./core/entity/dna"
POPULATION_FILE="$DNA_DIR/population.dat"

# Inicializar población si no existe
if [ ! -f "$POPULATION_FILE" ]; then
    # Creamos 3 entidades primordiales con ADN aleatorio (Velocidad, Fuerza, Resistencia, Inteligencia)
    echo "Alpha-Unit:[A1-S9-R4-I8]:100" > "$POPULATION_FILE"
    echo "Beta-Unit:[A3-S5-R7-I6]:100" >> "$POPULATION_FILE"
    echo "Omega-Unit:[A8-S2-R2-I9]:100" >> "$POPULATION_FILE"
fi

clear
echo "=================================================="
echo "      🧬 CHRONOOS: ECOSYSTEM DNA EVOLUTION 🧬      "
echo "=================================================="
echo "Entidades vivas detectadas en el sandbox local:\n"

while IFS= read -r line; do
    NAME=$(echo "$line" | cut -d: -f1)
    DNA=$(echo "$line" | cut -d: -f2)
    ENERGY=$(echo "$line" | cut -d: -f3)
    
    # Representación visual en 2D (muñequito ASCII dinámico según su ADN)
    echo " [O_O] Entidad: $NAME"
    echo "       ADN:     $DNA"
    echo "       Energía: ${ENERGY}%"
    echo "       Visual:  o--<@>--o (Activo en Termux)"
    echo "--------------------------------------------------"
done < "$POPULATION_FILE"

# Simulación de mutación y evolución generacional
echo "\n[*] Ejecutando ciclo de mutación y selección natural..."
TEMP_FILE="$DNA_DIR/temp_pop.dat"

while IFS= read -r line; do
    NAME=$(echo "$line" | cut -d: -f1)
    DNA=$(echo "$line" | cut -d: -f2)
    ENERGY=$(echo "$line" | cut -d: -f3)
    
    # Mutación aleatoria simple de energía y resistencia
    NEW_ENERGY=$((ENERGY - 5 + (RANDOM % 15)))
    if [ "$NEW_ENERGY" -gt 100 ]; then NEW_ENERGY=100; fi
    if [ "$NEW_ENERGY" -lt 10 ]; then NEW_ENERGY=10; fi
    
    echo "$NAME:$DNA:$NEW_ENERGY" >> "$TEMP_FILE"
done < "$POPULATION_FILE"

mv "$TEMP_FILE" "$POPULATION_FILE"
echo "[✓] Ciclo completado. Las entidades han procesado su ADN y evolucionado."
