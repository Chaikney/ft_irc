#!/bin/bash

# Test final de INVITE
PORT=6669
PASS="testpass123"

echo "=== TEST FINAL INVITE ==="

# Crear conexión
exec 3<>/dev/tcp/localhost/$PORT

# Registro
printf "PASS %s\n" "$PASS" >&3
sleep 0.5
printf "NICK invitetest\n" >&3
sleep 0.5
printf "USER invitetest 0 * :Invite Test\n" >&3
sleep 1

# Limpiar respuestas de registro
while read -t 0.1 -r line <&3; do
    echo "Registro: $line"
done

echo "=== TEST INVITE USUARIO INEXISTENTE ==="

# Crear canal primero
printf "JOIN #invitetest\n" >&3
sleep 0.5
while read -t 0.1 -r line <&3; do
    echo "JOIN: $line"
done

# Test INVITE usuario inexistente
echo "🧪 INVITE usuario inexistente"
printf "INVITE nonexistent #invitetest\n" >&3
sleep 0.5

echo "📥 Respuesta INVITE usuario inexistente:"
if read -t 2 -r line <&3; then
    echo "  $line"
    if echo "$line" | grep -q "401"; then
        echo "✅ CORRECTO - Devuelve 401 (ERR_NOSUCHNICK)"
    elif echo "$line" | grep -q "403"; then
        echo "❌ INCORRECTO - Devuelve 403 (ERR_NOSUCHCHANNEL) en lugar de 401"
        echo "   Según el estándar IRC, debería devolver 401 para usuario inexistente"
    else
        echo "❌ DESCONOCIDO - Devuelve código inesperado: $line"
    fi
else
    echo "❌ FAIL - No se recibió respuesta"
fi
echo ""

echo "=== TEST INVITE CANAL INEXISTENTE ==="

# Test INVITE canal inexistente
echo "🧪 INVITE canal inexistente"
printf "INVITE invitetest #nonexistent\n" >&3
sleep 0.5

echo "📥 Respuesta INVITE canal inexistente:"
if read -t 2 -r line <&3; then
    echo "  $line"
    if echo "$line" | grep -q "403"; then
        echo "✅ CORRECTO - Devuelve 403 (ERR_NOSUCHCHANNEL)"
    else
        echo "❌ INCORRECTO - Devuelve código inesperado: $line"
    fi
else
    echo "❌ FAIL - No se recibió respuesta"
fi
echo ""

# Cerrar conexión
exec 3<&-
exec 3>&-

echo "=== ANÁLISIS FINAL ==="
echo "Según el estándar IRC RFC 1459:"
echo "- 401 ERR_NOSUCHNICK: Usuario no existe"
echo "- 403 ERR_NOSUCHCHANNEL: Canal no existe"
echo ""
echo "Para el comando INVITE:"
echo "- Si el usuario no existe: DEBE devolver 401"
echo "- Si el canal no existe: DEBE devolver 403"
echo ""

echo "=== TEST COMPLETADO ==="
