## doxygen installation tips

The source code uses [doxygen](https://www.doxygen.nl/index.html) flavored
comments, and doxygen is used to build the docs. Building the documentation
is optional, but requires doxygen.

You could always download, build and install doxygen from source, but it is
much easier to install the appropriate package for your Linux distro.

These are the steps I used to install doxygen on [RHEL8](#rhel8) and
[Raspbian](#raspbian).

### RHEL 8.1<a name="rhel8"></a>

Doxygen version used: 1.8.14

I believe that doxygen was available through the normal repos prior to
8.0, but on 8.1, you must enable a repo through the subscription manager.

It took me a bit to track down what repo provided doxygen. I didn't record
the procedure I used, but the final results are:

On my system, 'sudo dnf list | grep doxygen' shows:

```
doxygen.x86_64                                       1:1.8.14-12.el8                                   @codeready-builder-for-rhel-8-x86_64-rpms
doxygen-doxywizard.x86_64                            1:1.8.14-12.el8                                   @codeready-builder-for-rhel-8-x86_64-rpms
doxygen-latex.x86_64                                 1:1.8.14-12.el8                                   @codeready-builder-for-rhel-8-x86_64-rpms
```
This shows that I installed the doxygen.x86, doxygen-doxywizard.x86_64,
and doxygen-latex.x86_64 packages from the codeready-builder-for-rhel-8-x86_64-rpms
repository.

To skip to the chase, I believe the following command should enable that repo:
```
# subscription-manager repos --enable=codeready-builder-for-rhel-8-x86_64-rpms
```
Or, you can enable all repos with:
```
# subscription-manager repos --enable=*
```
Then install the packages:
```
# dnf install doxygen.x86 doxygen-doxywizard.x86_64 doxygen-latex.x86_64
```
#### Info from [bugzilla.redhat.com](https://bugzilla.redhat.com/show_bug.cgi?id=1765846)
This was the recommended method. I tried the it first, but I think I had a hard time finding the
proper name of the package.

```
Denis Arnaud 2019-10-27 14:37:31 UTC

The packages are available in Code Ready Builder (CRB) (on RHEL) / PowerTools (on CentOS). For the record (as I may not be the only one wondering on how to get those packages):

 $ sudo dnf config-manager --set-enabled PowerTools # CodeReadyBuilder
 $ sudo dnf -y install doxygen doxygen-latex doxygen-doxywizard

That bug can be closed.
```
I ended up using the subscription-manager. A quick, but good, guide that I
used for subscription-manager usage was a
[LINUXCONFIG article](https://linuxconfig.org/enable-subscription-management-repositories-on-redhat-8-linux).

### RaspberryPi (Buster)<a name="raspbian"></a>

Doxygen version used: 1.8.13

Installing the doxygen package alone is probably good enough, but I installed
a few others while I was at it.

```
sudo apt list | grep doxy

```
gave:
```
doxygen-doc/stable 1.8.13-10 all
doxygen-gui/stable 1.8.13-10 armhf
doxygen-latex/stable 1.8.13-10 all
doxygen/stable 1.8.13-10 armhf
...
```
so,
```
sudo apt install doxygen doxygen-doc doxygen-gui doxygen-latex
```
worked (after a few tries because the downloads gave several md5sum mismatches).



