# centralized script to be invoked whenever doing an automated server-side
# pull, to make sure we are doing all the right steps to ensure
# that our local repo is an exact clone of the remote, even as things
# are deleted on the remote


# in case we are on some other branch, or rolled back from the tip
git checkout master

# delete local tags, so we can pull latest tags from remote
# this allows remote to delete tags, and have them be deleted locally
git tag -d $(git tag)


# pull tags again fresh, and use force to get changes made through force-pushes
git pull --tags --force


# This will roll our local repo back to match the remote, in the case
# of a force-push that deleted some later commits that we still have locally
git reset origin/master --hard
