# GitHub Wiki publishing

The English Wiki source is stored in the repository `/wiki` directory.

## One-time GitHub setup

1. Open the repository on GitHub.
2. Open **Settings > General > Features** and enable **Wikis**.
3. Open the **Wiki** tab and create the first `Home` page once. This initializes the separate `<repository>.wiki.git` repository.
4. Open **Settings > Actions > General > Workflow permissions**.
5. Select **Read and write permissions** and save.
6. Commit and push the `/wiki` directory and `.github/workflows/publish-wiki.yml` to `main` or `master`.

The workflow runs automatically whenever a file under `/wiki` changes. It can also be started manually from **Actions > Publish GitHub Wiki > Run workflow**.

## Optional WIKI_TOKEN secret

The workflow first tries the repository `GITHUB_TOKEN`. If organization policy prevents that token from pushing to the Wiki repository, create a fine-grained personal access token with repository contents write access and add it as:

```text
Repository Settings > Secrets and variables > Actions > New repository secret
Name: WIKI_TOKEN
```

Never commit the token into the repository.

## Source-of-truth behavior

The workflow mirrors the root files from `/wiki` into the separate Wiki Git repository. Files deleted from `/wiki` are also deleted from the published Wiki on the next run.

GitHub Wiki page names are derived from Markdown filenames. Keep all pages flat in `/wiki`, for example:

```text
wiki/Home.md
wiki/Getting-Started.md
wiki/_Sidebar.md
wiki/_Footer.md
```

## Local manual publication

A maintainer may also clone the Wiki repository and copy the files manually:

```powershell
git clone https://github.com/OWNER/REPOSITORY.wiki.git
Copy-Item -Recurse -Force .\wiki\* .\REPOSITORY.wiki\
cd .\REPOSITORY.wiki
git add --all
git commit -m "Update English manual"
git push
```

Automatic publication is preferred because it keeps documentation versioned with the firmware source.
