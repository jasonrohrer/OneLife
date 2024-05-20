# centralized script to be invoked whenever doing an automated server-side
# pull, to make sure we are doing all the right steps to ensure
# that our local repo is an exact clone of the remote, even as things
# are deleted on the remote


# in case we are on some other branch, or rolled back from the tip
git checkout master

# delete local tags, so we can pull latest tags from remote
# this allows remote to delete tags, and have them be deleted locally
# do this silently, so we don't blast the full tag list to output
git tag -d $(git tag) >/dev/null 2>&1


# re-fetch the tags quietly too, to re-get all tags from remote
git fetch --tags >/dev/null 2>&1


# pull, and use force to get changes made through force-pushes
git pull --force


# This will roll our local repo back to match the remote, in the case
# of a force-push that deleted some later commits that we still have locally
git reset origin/master --hard
