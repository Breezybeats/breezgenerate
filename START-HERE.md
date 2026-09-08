# Get your Mac M1 VST3 — no Xcode on your Mac

This folder is the project you upload, not an already compiled plugin. The GitHub workflow builds `NEUROJ.vst3` on an Apple Silicon Mac, tests it, and publishes a downloadable build artifact. You do not need to edit C++.

## 1. Create the repository

Sign in to GitHub → **+** → **New repository**. Name it `neuroj`. Public standard-runner builds are free; the source is public. A private repository uses your account's included Actions allowance; check spending settings before building. You do not have to make your work public. Add a README to initialise the repository, then **Create repository**.

## 2. Upload the CONTENTS of this folder

Unzip the downloaded project. Open the `NEUROJ-GitHub` folder in Finder.

Press **Command + Shift + . (period)** so Finder shows `.github` and `.gitignore`. The `.github` folder is essential: it contains the automatic Mac build recipe.

In GitHub choose **Add file → Upload files**. Drag everything **inside** `NEUROJ-GitHub` onto that page, including `.github`, `Source`, `Tests`, `CMakeLists.txt`, and the documentation. Do not upload the ZIP itself or an outer `NEUROJ-GitHub` folder. Choose **Commit changes**.

At the top level of your repository, you should see `CMakeLists.txt`, `Source`, `Tests`, and `.github`.

If GitHub did not accept the `.github` folder: **Add file → Create new file**. Name the file exactly `.github/workflows/build-mac.yml`, paste the full contents of the supplied `BUILD-MAC-WORKFLOW.txt`, and commit. You only need this fallback if the workflow is missing.

## 3. Wait for the build

Open **Actions → Build Mac M1 VST3**. The upload should start a run automatically on `main` or `master`. Otherwise choose **Run workflow → Run workflow**. If GitHub asks to enable Actions, enable them for this repository.

Wait for a green check. The job includes dependency downloads, compilation, DSP tests, state tests, and a real VST3 scan/load test. A build may take several minutes; the workflow stops after 40 minutes.

If there is a red X, open the failed step and send its error log back. Do not install an artifact from a failed run. This workflow has not been executed on a Mac in the creation environment; its first successful cloud run is a required validation step.

## 4. Download

Open the successful workflow run. Scroll to **Artifacts**, click **NEUROJ-Mac-M1-VST3**, unzip the download, then unzip the inner `NEUROJ-Mac-M1.zip`. Keeping the inner ZIP preserves the Mac bundle and ad-hoc signature.

You should now have a folder containing **NEUROJ.vst3**, installation instructions, notices, and an audio preview. Follow `INSTALL-MAC.md`.

## What you still need to test

Only your Mac can confirm FL Studio scanning, playback, project reopen, automation, offline export, and sound quality in your actual setup. The GitHub test is a JUCE host test, not FL Studio certification.

Official references:

- [GitHub runner platforms and public/private billing behaviour](https://docs.github.com/en/actions/reference/runners/github-hosted-runners)
- [Downloading build artifacts](https://docs.github.com/en/actions/managing-workflow-runs-and-deployments/managing-workflow-runs/downloading-workflow-artifacts)
