#!/bin/sh

echo "🚀 Starting rsync to sync project to bezos:/var/www/communication/ ..."
rsync -rv --exclude env --exclude __pycache__ --exclude .mypy_cache communication/ bezos:/var/www/communication/
if [ $? -eq 0 ]; then
  echo "✅ Sync complete!"
else
  echo "❌ Rsync failed! Aborting."
  exit 1
fi

echo "🔌 Activating remote virtualenv and installing requirements..."
ssh bezos '
  echo "🐍 Activating virtualenv..."
  . /var/www/communication/env/bin/activate
  echo "📦 Installing Python dependencies from requirements.txt..."
  pip3 install -r /var/www/communication/requirements.txt
  if [ $? -eq 0 ]; then
    echo "🎉 Dependencies installed successfully!"
  else
    echo "⚠️  Failed to install dependencies!"
    exit 1
  fi
'

