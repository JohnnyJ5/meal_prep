#!/bin/bash

# Ensure we exit on error
set -e

PROJECT_ID=$(gcloud config get-value project)
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
    --set-env-vars="DB_PATH=/mnt/db/meals.db" \
    --add-volume=name=db-volume,type=cloud-storage,bucket=meal-prep-db-bucket \
    --add-volume-mount=volume=db-volume,mount-path=/mnt/db

echo "----------------------------------------"
echo "Deployment complete!"
echo ""
echo "You can now execute the job to list tables:"
echo "  gcloud run jobs execute ${JOB_NAME} --region ${REGION} --args=\"/mnt/db/meals.db,-l\""
echo ""
echo "To dump a specific table (e.g., meals):"
echo "  gcloud run jobs execute ${JOB_NAME} --region ${REGION} --args=\"/mnt/db/meals.db,-d,meals\""
echo "----------------------------------------"
