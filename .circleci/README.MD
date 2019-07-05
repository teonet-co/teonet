## Description circle-ci config.yaml

This is a new version of the configuration, old is available here https://github.com/n-semenov-wg/yandex-cloud/blob/master/circle/teonet-co.yaml

Docker image which is used to assemble is taken in cr.yandex/crp8ph86e05bgnsgjbb8/ci_ubuntu_build_docker (see in circle config, job: tagged-build-ubuntu). Based on it, the commands are executed on "step" block in circle-ci config. If the step is successful - go to step two (see in circle config, job: tagged-build-docker).
 
Assembled docker container pushing to: cr.yandex/crp8ph86e05bgnsgjbb8/test/teonet (the path will be changed later). In https://circleci.com/ build starts by TAG. For this, you need to create a release with any tag. Build start by commit and untagget build not used.

**Get images from yandex container regisrty (YCR) like this:**

>docker login \
>--username oauth\
>--password $YC_AUTH \ #TOKEN ID you can ask from me
>cr.yandex

>docker pull cr.yandex/crp8ph86e05bgnsgjbb8/path/to/image

**in our case:**
>docker pull  cr.yandex/crp8ph86e05bgnsgjbb8/test/teonet
