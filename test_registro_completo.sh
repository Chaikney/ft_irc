#!/bin/bash

# Test completo del registro
PORT=6669
PASS="testpass123"

echo "=== TEST REGISTRO COMPLETO ==="

# Crear conexión
exec 3<>/dev/tcp/localhost/$PORT

echo "🧪 Enviando comandos de registro"
printf "PASS %s\n" "$PASS" >&3
sleep 0.2
printf "NICK testuser\n" >&3
sleep 0.2
printf "USER testuser 0 * :Test User\n" >&3
sleep 1

echo "📥 Todas las respuestas de registro:"
while read -t 0.5 -r line <&3; do
    echo "  $line"
done

echo "🧪 Enviando PING"
printf "PING :test\n" >&3
sleep 0.5

echo "📥 Respuesta PING:"
while read -t 1 -r line <&3; do
    echo "  $line"
    if echo "$line" | grep -q "PONG"; then
        echo "✅ PING funciona correctamente"
    fi
done

echo "🧪 Enviando INVITE usuario inexistente"
printf "INVITE nonexistent #test\n" >&3
sleep 0.5

echo "📥 Respuesta INVITE:"
while read -t 1 -r line <&3; do
    echo "  $line"
    if echo "$line" | grep -q "401"; then
        echo "✅ CORRECTO - Devuelve 401 (ERR_NOSUCHNICK)"
    elif echo "$line" | grep -q "403"; then
        echo "❌ INCORRECTO - Devuelve 403 (ERR_NOSUCHCHANNEL) en lugar de 401"
    fi
done

# Cerrar conexión
exec 3<&-
exec 3>&-

echo "=== TEST COMPLETADO ==="
