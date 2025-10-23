#!/bin/bash

# Test simple del arreglo de INVITE
PORT=6670
PASS="testpass123"

echo "=== TEST INVITE SIMPLE ==="

# Crear conexión
exec 3<>/dev/tcp/localhost/$PORT

echo "🧪 Registrando usuario"
printf "PASS %s\n" "$PASS" >&3
sleep 0.2
printf "NICK testuser\n" >&3
sleep 0.2
printf "USER testuser 0 * :Test User\n" >&3
sleep 1

# Leer todas las respuestas de registro
echo "📥 Respuestas de registro:"
while read -t 0.5 -r line <&3; do
    echo "  $line"
done

echo "🧪 Test INVITE usuario inexistente"
printf "INVITE nonexistent #test\n" >&3
sleep 0.5

echo "📥 Respuesta INVITE:"
if read -t 2 -r line <&3; then
    echo "  $line"
    if echo "$line" | grep -q "401"; then
        echo "✅ CORRECTO - Devuelve 401 (ERR_NOSUCHNICK)"
    elif echo "$line" | grep -q "403"; then
        echo "❌ INCORRECTO - Devuelve 403 (ERR_NOSUCHCHANNEL)"
    else
        echo "❌ DESCONOCIDO - Devuelve: $line"
    fi
else
    echo "❌ FAIL - No se recibió respuesta"
fi

# Cerrar conexión
exec 3<&-
exec 3>&-

echo "=== TEST COMPLETADO ==="
