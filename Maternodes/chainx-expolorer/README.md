CHAINX BLOCKCHAIN EXPOLORER

Chainx-mnTax

Simple bash script for listing masternode payments in USD.
Usage:

    save dash-mnTax.sh and execute chmod 755 chainx-mnTax.sh
    Usage: bash chainx-mnTax.sh <chainx> [<START chainx> <END DATE>]
    Example: bash chainx-mnTax.sh RXXdEmOjfadjfiieur8fjdkewi7849jfdls "2017-01-01 00:00:00" "2017-12-31 23:59:59"

Description:

This script will list all masternode payments (earliest starting August 2017) and their US Dollar value at that given time.

Algorithm:

    Get MN transaction data from ChainX block explorer
    
    Printing History and calculating Total USD Income

Initial release
