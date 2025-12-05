FROM python:3.11-alpine

WORKDIR /app

RUN apk add --no-cache gcc musl-dev linux-headers

COPY requirements.txt .

RUN pip install --no-cache-dir -r requirements.txt

COPY . .

EXPOSE 8083

HEALTHCHECK --interval=30s --timeout=10s --start-period=70s --retries=3 \
    CMD python -c "import redis; r = redis.Redis.from_url('redis://:redis123@redis:6379/2'); r.ping()" || exit 1

CMD ["python", "bot.py"]