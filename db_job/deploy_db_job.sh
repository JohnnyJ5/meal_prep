#!/bin/bash

# Ensure we exit on error
set -e

DB_PATH="/mnt/db/meals.db"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --db-path)
            DB_PATH="$2"
            shift 2
            ;;
        *)
            echo "Unknown argument: $1"
            echo "Usage: $0 [--db-path <path>]"
            exit 1
            ;;
    esac
done

PROJECT_ID=mealprepsite
REGION="us-central1"
JOB_NAME="db-dump-job"
IMAGE_URL="gcr.io/${PROJECT_ID}/${JOB_NAME}"

echo "----------------------------------------"
echo "Building Docker image for the job..."
echo "----------------------------------------"
docker build -t ${IMAGE_URL} -f Dockerfile.job .

echo "----------------------------------------"
echo "Configuring Docker authentication..."
echo "----------------------------------------"
gcloud auth configure-docker gcr.io --quiet

echo "----------------------------------------"
echo "Pushing Docker image to Google Container Registry..."
echo "----------------------------------------"
docker push ${IMAGE_URL}

echo "----------------------------------------"
echo "Deploying Cloud Run Job '${JOB_NAME}'..."
echo "----------------------------------------"
gcloud run jobs deploy ${JOB_NAME} \
    --image ${IMAGE_URL} \
    --region ${REGION} \
    --set-env-vars="DB_PATH=${DB_PATH}" \
    --add-volume=name=db-volume,type=cloud-storage,bucket=meal-prep-db-bucket \
    --add-volume-mount=volume=db-volume,mount-path=/mnt/db

echo "----------------------------------------"
echo "Deployment complete! DB_PATH=${DB_PATH}"
echo ""
echo "List tables:"
echo "  gcloud run jobs execute ${JOB_NAME} --region ${REGION} --args=\"-l\""
echo ""
echo "Dump a table (e.g., meals):"
echo "  gcloud run jobs execute ${JOB_NAME} --region ${REGION} --args=\"-d,meals\""
echo ""
echo "Execute SQL:"
echo "  gcloud run jobs execute ${JOB_NAME} --region ${REGION} --args=\"-e,UPDATE meals SET name='X' WHERE id=1\""
echo "----------------------------------------"
