# Google Cloud Platform Commands

This guide provides targeted `gcloud` and `gsutil` commands for the Meal Prep application.

## Development & OAuth

### Fixing "Error 403: access_denied" (OAuth Testing Mode)
If you encounter `Error 403: access_denied` with the message "The app is currently being tested, and can only be accessed by developer-approved testers" when testing Google login, it means your GCP project's OAuth Consent Screen is in "Testing" mode, and the email you are trying to log in with is not authorized.

**To fix this via the Google Cloud Console:**
1. Go to the [Google Cloud Console](https://console.cloud.google.com/).
2. Select your project.
3. Navigate to **APIs & Services > OAuth consent screen**.
4. Scroll down to the **Test users** section.
5. Click **+ ADD USERS**.
6. Enter the email address you are using to sign in.
7. Click **SAVE**.

Alternatively, if your app is ready to be used by anyone, you can click **PUBLISH APP** on the OAuth consent screen to move its status from "Testing" to "In production", which removes the requirement to manually add test users.

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

### Set Up Token Encryption Key
The `MEAL_PREP_TOKEN_KEY` environment variable is required for AES-256-GCM encryption of stored OAuth tokens. Generate and store it as a Cloud Secret:
```bash
# Generate a 256-bit key
openssl rand -hex 32

# Create the secret in Secret Manager
echo -n "YOUR_64_HEX_CHARS" | gcloud secrets create meal-prep-token-key --data-file=-

# Grant Cloud Run service account access
gcloud secrets add-iam-policy-binding meal-prep-token-key \
    --member="serviceAccount:[PROJECT_NUMBER]-compute@developer.gserviceaccount.com" \
    --role="roles/secretmanager.secretAccessor"
```
Then mount the secret as an env var in `cloudbuild.yaml` / Cloud Run configuration as `MEAL_PREP_TOKEN_KEY`.

### Grant Secret Access to Cloud Run
Allow the default compute service account to access a specific secret.
```bash
gcloud secrets add-iam-policy-binding [SECRET_NAME] \
    --member="serviceAccount:[PROJECT_NUMBER]-compute@developer.gserviceaccount.com" \
    --role="roles/secretmanager.secretAccessor"
```

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

## Database Admin Job

### Deploy Database Dump Job
Builds and deploys the Cloud Run Job `db-dump-job` to run `dump_db.py`.
```bash
./deploy_db_job.sh
```

### Run Database Dump Job (List Tables)
Runs the `db-dump-job` to output the list of tables to Cloud Run logs.
```bash
gcloud run jobs execute db-dump-job --region=us-central1 --args="/mnt/db/meals.db,-l"
```

### Run Database Dump Job (Dump Table)
Runs the job to dump the contents of a specific table, e.g. `meals`.
```bash
gcloud run jobs execute db-dump-job --region=us-central1 --args="/mnt/db/meals.db,-d,meals"
```

### Run Database Admin Job (Execute SQL)
Runs an INSERT, UPDATE, or DELETE statement directly against the production database.
```bash
gcloud run jobs execute db-dump-job --region=us-central1 \
    --args="/mnt/db/meals.db,-e,UPDATE meals SET name='New Name' WHERE id=1"
```

### View Job Logs
Watch the logs of the executed job.
```bash
gcloud run jobs logs tail db-dump-job --region=us-central1
```

### Running Jobs via the Cloud Console UI
You can execute any of the above jobs without the CLI:
1. Go to **Cloud Run > Jobs** and select `db-dump-job`.
2. Click **Execute**.
3. Expand **"Show advanced settings"** and find the **"Override"** section.
4. In the **Arguments** field, enter each arg on its own line — e.g.:
   ```
   /mnt/db/meals.db
   -e
   UPDATE meals SET name='New Name' WHERE id=1
   ```
5. Click **Execute**. View output under **Executions > Logs**.
