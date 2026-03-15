# Google Cloud Cheat Sheet - Meal Prep App

This document contains useful `gcloud` commands for managing, deploying, and troubleshooting your meal prep application.

## 🚀 Deployment & Updates

### Deploy a new version
Run this in the `meal_prep` directory after making code changes:
```bash
gcloud builds submit --config cloudbuild.yaml
```

### Set default project (if not already set)
```bash
gcloud config set project [YOUR_PROJECT_ID]
```

## 🔍 Monitoring & Logs

### View real-time logs in terminal
```bash
gcloud run services logs tail meal-prep --region=us-central1
```

### Check service status
```bash
gcloud run services describe meal-prep --region=us-central1
```

## 🔒 Security & Access

### Make service private (authentication required)
```bash
gcloud run services remove-iam-policy-binding meal-prep \
    --region=us-central1 \
    --member="allUsers" \
    --role="roles/run.invoker"
```

### Make service public (no login required)
```bash
gcloud run services add-iam-policy-binding meal-prep \
    --region=us-central1 \
    --member="allUsers" \
    --role="roles/run.invoker"
```

### Authenticated Proxy (Access private site locally)
Run this command, then visit the URL it provides (usually `localhost:8080`):
```bash
gcloud run services proxy meal-prep --region=us-central1
```

## 📦 Database & Storage (GCS)

### List files in your bucket
```bash
gsutil ls gs://[YOUR_BUCKET_NAME]/
```

### Download latest `meals.db` to local machine
```bash
gsutil cp gs://[YOUR_BUCKET_NAME]/meals.db ./meals.db_backup
```

### Upload a local database to the bucket
```bash
gsutil cp ./meals.db gs://[YOUR_BUCKET_NAME]/meals.db
```

## 🛠 Troubleshooting

### SSH into a temporary build environment (if build fails)
```bash
gcloud builds submit --config cloudbuild.yaml --no-cache
```

### List all gcloud configurations
```bash
gcloud config list
```
