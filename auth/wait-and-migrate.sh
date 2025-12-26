#!/bin/sh
# wait-and-migrate.sh
set -e

host="$1"
port="$2"
user="$3"
password="$4"
db="$5"
migrations_dir="$6"
shift 6
cmd="$@"

echo "⏳ Waiting for PostgreSQL to be ready..."
until PGPASSWORD="$password" psql -h "$host" -p "$port" -U "$user" -d "$db" -c '\q' 2>/dev/null; do
  echo "📡 PostgreSQL is unavailable - sleeping"
  sleep 2
done

echo "✅ PostgreSQL is up - checking for migrations..."

# Выполняем миграции если есть файлы
if [ -d "$migrations_dir" ] && [ -n "$(ls -A $migrations_dir/*.sql 2>/dev/null)" ]; then
  echo "🔄 Running migrations from $migrations_dir..."
  for migration_file in $migrations_dir/*.sql; do
    if [ -f "$migration_file" ]; then
      echo "📄 Executing: $(basename $migration_file)"
      PGPASSWORD="$password" psql -h "$host" -p "$port" -U "$user" -d "$db" -f "$migration_file"
    fi
  done
  echo "✅ Migrations completed"
else
  echo "⚠️  No migrations found in $migrations_dir"
fi

echo "🚀 Starting application: $cmd"
exec $cmd