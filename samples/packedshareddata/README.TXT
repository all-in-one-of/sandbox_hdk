== What this is ==

A sample of a packed primitive that saves and loads custom shared data
in a .bgeo file.

== How to build ==

hcustom GU_PackedSharedData.C
hcustom -s packedshareddata.C

== How to test ==

If you create a .geo file with a packedshareddata primitive in it, you
can test by running "vexexec" with the VEX function.

-----------------------------------------------------------------
/*%vexexec %
 */
cvex
show(string filename="packedshareddata.geo")
{
    string	name = primintrinsic(filename, "typename", 0);
    string	date = primintrinsic(filename, "creation_date", 0);
    string	host = primintrinsic(filename, "creation_host", 0);
    printf("Name: %s\n", name);
    printf("Date: %s\n", date);
    printf("Host: %s\n", host);
}
-----------------------------------------------------------------

Or, you can test by loading into Houdini (of course).
