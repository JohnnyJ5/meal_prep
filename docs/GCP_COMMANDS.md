# Google Cloud Platform Commands

This guide provides targeted `gcloud` and `gsutil` commands for the Meal Prep application.

## Deployment

### Submit a Cloud Build
Builds the container and deploys to Cloud Run using the `cloudbuild.yaml` configuration.
```bash
gcloud builds submit --config cloudbuild.yaml
```

### List Projects
Get a list of all projects you have access to.
```bash
gcloud projects list
```

### Set Default Project
```bash
gcloud config set project [YOUR_PROJECT_ID]
```

## Monitoring & Management

### Tail Service Logs
Watch the real-time output from the Cloud Run service.
```bash
gcloud run services logs tail meal-prep --region=us-central1
```

### Describe Service
Get details about the current deployment, including the URL and status.
```bash
gcloud run services describe meal-prep --region=us-central1
```

## Security

### Make Service Private
Remove public access to the API.
```bash
gcloud run services remove-iam-policy-binding meal-prep \
    --region=us-central1 \
    --member="allUsers" \
    --role="roles/run.invoker"
```

### Make Service Public
Enable unauthenticated access.
```bash
gcloud run services add-iam-policy-binding meal-prep \
    --region=us-central1 \
    --member="allUsers" \
    --role="roles/run.invoker"
```

### Local Proxy
Access a private Cloud Run service from your local machine.
```bash
gcloud run services proxy meal-prep --region=us-central1
```

## Storage (GCS Sync)

### List Bucket Contents
```bash
gsutil ls gs://[YOUR_BUCKET_NAME]/
```

### Backup Database
Download the production database locally.
```bash
gsutil cp gs://[YOUR_BUCKET_NAME]/meals.db ./meals.db_backup
```

### Restore/Upload Database
```bash
gsutil cp ./meals.db gs://[YOUR_BUCKET_NAME]/meals.db
```
