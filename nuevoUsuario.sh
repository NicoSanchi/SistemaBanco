#!/bin/bash

# ✅ Verificar permisos
if [[ "$EUID" -ne 0 ]]; then
  echo "❌ Este script debe ejecutarse como root. Usa: sudo $0"
  exit 1
fi

# Ruta de banco basada en la ubicación de este script
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROGRAMA_ORIGEN="$SCRIPT_DIR/banco"

# 🔍 Verificar que el programa exista
if [ ! -f "$PROGRAMA_ORIGEN" ]; then
    echo "❌ El archivo '$PROGRAMA_ORIGEN' no existe. Asegúrate de que 'banco' esté en la misma carpeta que este script."
    exit 1
fi

# 👤 Solicitar nombre de usuario
read -p "Introduce el nombre del nuevo usuario: " USUARIO

# ❌ Comprobar si el usuario ya existe
if id "$USUARIO" &>/dev/null; then
    echo "❌ El usuario '$USUARIO' ya existe. Aborta el script."
    exit 1
fi

# 🔒 Solicitar contraseña
read -s -p "Introduce la contraseña para el nuevo usuario: " CONTRASENA
echo
read -s -p "Confirma la contraseña: " CONTRASENA_CONFIRMACION
echo

if [ "$CONTRASENA" != "$CONTRASENA_CONFIRMACION" ]; then
    echo "❌ Las contraseñas no coinciden. Aborta el script."
    exit 1
fi

# ✅ Crear usuario
useradd -m -s /bin/bash "$USUARIO"
echo "$USUARIO:$CONTRASENA" | chpasswd

# 📂 Crear directorio destino para el programa
DESTINO_PROGRAMA="/home/$USUARIO/SistemaBanco"
mkdir -p "$DESTINO_PROGRAMA"
cp "$PROGRAMA_ORIGEN" "$DESTINO_PROGRAMA/"
chmod +x "$DESTINO_PROGRAMA/banco"
chown -R "$USUARIO:$USUARIO" "$DESTINO_PROGRAMA"

# 🖥 Crear Escritorio si no existe
ESCRITORIO="/home/$USUARIO/Escritorio"
mkdir -p "$ESCRITORIO"
chown "$USUARIO:$USUARIO" "$ESCRITORIO"

# 🔗 Crear acceso directo
ARCHIVO_DESKTOP="$ESCRITORIO/banco.desktop"

cat <<EOF > "$ARCHIVO_DESKTOP"
[Desktop Entry]
Name=banco
Exec=$DESTINO_PROGRAMA/banco
Icon=utilities-terminal
Type=Application
Terminal=true
EOF

chmod +x "$ARCHIVO_DESKTOP"
chown "$USUARIO:$USUARIO" "$ARCHIVO_DESKTOP"

echo "✅ Usuario '$USUARIO' creado con acceso a 'banco' y acceso directo en su escritorio."
echo "------------------------------------------------"
echo "1. Cierra sesión"
echo "2. Inicia sesión con '$USUARIO'"
echo "3. Mira el escritorio"
echo "4. Ejecuta Secure Bank"
echo "5. Disfruta de todas las funcionalidades disponibles"
echo "------------------------------------------------"