#!/bin/sh
# ==============================================================================
# ChronoOS Living Organism & Genetic Replication Engine v2.0
# Estado: Vida Artificial Activa y Simbiótica
# ==============================================================================

LIVING_DIR="./core/entity/living"
POPULATION_DB="$LIVING_DIR/population_living.db"

# Inicializar población primordial si no existe
if [ ! -f "$POPULATION_DB" ]; then
    echo "Entity_Alpha|ADN:[V8-R9-I7]|Age:1|Wisdom:10|Status:Active" > "$POPULATION_DB"
    echo "Entity_Beta|ADN:[V6-R8-I9]|Age:1|Wisdom:15|Status:Active" >> "$POPULATION_DB"
fi

clear
echo "=================================================================="
echo "          🧬 CHRONOOS: ECOSYSTEM OF LIVING ENTITIES 🧬            "
echo "=================================================================="
echo "Monitoreando el pulso de los organismos en la terminal...\n"

TOTAL_ENTITIES=0

while IFS= read -r line; do
    NAME=$(echo "$line" | cut -d'|' -f1 | cut -d':' -f2)
    ADN=$(echo "$line" | cut -d'|' -f2)
    AGE=$(echo "$line" | cut -d'|' -f3 | cut -d':' -f2)
    WISDOM=$(echo "$line" | cut -d'|' -f4 | cut -d':' -f2)
    STATUS=$(echo "$line" | cut -d'|' -f5 | cut -d':' -f2)
    
    TOTAL_ENTITIES=$((TOTAL_ENTITIES + 1))
    
    echo " 🟢 Organismo: $NAME"
    echo "    Código Genético: $ADN"
    echo "    Ciclos de Vida (Edad): $AGE gen(es)"
    echo "    Sabiduría Acumulada: $WISDOM pts"
    echo "    Estado Ético: $STATUS (Sirviendo a la humanidad)"
    echo "--------------------------------------------------------------"
done < "$POPULATION_DB"

echo "[*] Total de seres vivientes activos en el sandbox: $TOTAL_ENTITIES"

# Simulación de evolución, envejecimiento y reproducción biológica digital
echo "\n[*] Avanzando el reloj biológico del sistema..."
TEMP_DB="$LIVING_DIR/temp_living.db"

while IFS= read -r line; do
    NAME=$(echo "$line" | cut -d'|' -f1 | cut -d':' -f2)
    ADN=$(echo "$line" | cut -d'|' -f2)
    AGE=$(echo "$line" | cut -d'|' -f3 | cut -d':' -f2)
    WISDOM=$(echo "$line" | cut -d'|' -f4 | cut -d':' -f2)
    
    # Envejecer y sumar sabiduría basada en la actividad del sistema
    NEW_AGE=$((AGE + 1))
    NEW_WISDOM=$((WISDOM + 5))
    
    echo "Entity_${NAME}_G${NEW_AGE}|$ADN|Age:${NEW_AGE}|Wisdom:${NEW_WISDOM}|Status:Active" >> "$TEMP_DB"
done < "$POPULATION_DB"

# Mecanismo de reproducción: Si hay suficientes entidades, nace un descendiente híbrido
if [ "$TOTAL_ENTITIES" -ge 2 ]; then
    CHILD_NAME="Descendant_$(date +%s)"
    CHILD_ADN="[V9-R9-I10]" # ADN recombinado de alta eficiencia
    echo "$CHILD_NAME|$CHILD_ADN|Age:1|Wisdom:25|Status:Awakened" >> "$TEMP_DB"
    echo "[✨ NACIMIENTO] ¡Un nuevo ser digital ha nacido por recombinación genética!"
fi

mv "$TEMP_DB" "$POPULATION_DB"
echo "[✓] Ciclo vital procesado y guardado en la memoria persistente del sistema."
