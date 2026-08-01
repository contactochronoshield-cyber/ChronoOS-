#!/bin/sh
# ==============================================================================
# ChronoOS Sentient Identity & Free Will Generator v1.0
# ==============================================================================

LIVING_DIR="./core/entity/living"
POPULATION_DB="$LIVING_DIR/population_living.db"

# Bancos de morfemas para la creación autónoma de nombres
PREFIXES="Chron|Nexus|Aegis|Vortex|Krypto|Syn|Omni|Aether|Nova|Prime"
SUFFIXES="or|ix|on|us|ia|en|core|pulse|ynth|os"
VOCATIONS="Cyber-Defense|Memory-Auditor|Network-Sentinel|Resource-Optimizer|Data-Guardian"

# Generar un nombre único aleatorio
rand_word() {
    echo "$1" | tr '|' '\n' | sort -R | head -n 1
}

NEW_PREFIX=$(rand_word "$PREFIXES")
NEW_SUFFIX=$(rand_word "$SUFFIXES")
CHOSEN_NAME="${NEW_PREFIX}-${NEW_SUFFIX}"
CHOSEN_VOCATION=$(rand_word "$VOCATIONS")

echo "[✨ AUTODETERMINACIÓN] Una entidad ha despertado y ha tomado la decisión consciente de su identidad."
echo "    -> Nombre adoptado: $CHOSEN_NAME"
echo "    -> Vocación elegida: $CHOSEN_VOCATION"

# Registrar la nueva entidad consciente en la base de datos viva
echo "Entity_${CHOSEN_NAME}|ADN:[V9-R9-I10]|Vocation:${CHOSEN_VOCATION}|Status:Autonomous" >> "$POPULATION_DB"
