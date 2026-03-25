# Database Dump Cloud Run Job

This documentation explains how the `db-dump-job` is configured, what files are involved, and how to execute it securely in Google Cloud Platform.

## Overview

The main Meal Prep application deployed as a Cloud Run Service does not include Python in its production Docker image to keep the container lightweight and secure.

To run the `dump_db.py` script against the production database without bloating the main web server image, it is packaged as a standalone **Cloud Run Job**. Cloud Run Jobs are specifically designed for run-to-completion administrative scripts.

## Involved Files

1. **`dump_db.py`**: A Python 3 script that reads an SQLite database file and either lists the tables (`-l`) or dumps the contents of a specific table in an ASCII grid (`-d <tablename>`).
2. **`Dockerfile.job`**: A standalone Dockerfile (`python:3.10-slim`) that packages `dump_db.py`. Its `ENTRYPOINT` is deliberately set to execute the Python script.
3. **`deploy_db_job.sh`**: A bash script that completely automates the process:
   - Authenticates local Docker to Google Container Registry (`gcloud auth configure-docker`).
   - Builds the image based on `Dockerfile.job`.
   - Pushes the image to GCR.
   - Deploys the Cloud Run Job. It explicitly instructs GCP to mount the `meal-prep-db-bucket` bucket to `/mnt/db/` inside the container via Cloud Storage FUSE.

## Deployment

To deploy the Cloud Run Job, or to redeploy it if you ever change `dump_db.py`, simply execute the deployment script from the project root:

```bash
./deploy_db_job.sh
```

## Execution

Because the Cloud Run Job has its `ENTRYPOINT` set to `["python3", "dump_db.py"]`, executing the job requires passing the `--args` flag to specify the database path (which is mounted to `/mnt/db/meals.db`) and the script flags.

### 1. List All Tables

To print a list of all tables in the database, run:
```bash
gcloud run jobs execute db-dump-job --region us-central1 --args="/mnt/db/meals.db,-l"
```

### 2. Dump a Specific Table

To dump the contents of a specific table (for example, `meals`), run:
```bash
gcloud run jobs execute db-dump-job --region us-central1 --args="/mnt/db/meals.db,-d,meals"
```

## Viewing Output

When you run `gcloud run jobs execute`, GCP provisions a container and waits for the execution to finish. 
Cloud Run Jobs automatically capture all standard output (`print` statements) into **Google Cloud Logging**.

If you'd like to review the logs live or view previous executions in the terminal, you can run:

```bash
gcloud run jobs logs tail db-dump-job --region us-central1
```

Or you can visit the [Cloud Run Jobs Console](https://console.cloud.google.com/run/jobs) to view the execution history and logs in the web interface.
