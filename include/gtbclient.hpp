/*
 * GUARD the Bridge
 * Copyright (C) 2012  Matthew Finkel <Matthew.Finkel@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef gtbclient_h
#define gtbclient_h

class GTBClient {
  private:
    /** Car Number. */
    int ncarnum;

    /*8 IP Address of client */
    std::string ipaddr;

    /** Certificate client used for authenticated */
    gnutls_datum_t cert;

    /** Nightly Auth Code provided */
    std::string authcode;

    /** NetID of the Driver */
    std::string netidDriver;

    /** NetID of the Ride Along */
    std::string netidRideAlong;

    /** Number of seats in the car */
    int ncarsize;

    /** Accepted socket */
    int fdAccepted;

    /** Client has been verified */
    int verified;

  public:

    /** Default Constructor */
    GTBClient() {}

    /** Constructor: Set accepted socket */
    GTBClient(int acceptedFd)
    {
      fdAccepted = acceptedFd;
    }

    /* Accessors */

    /** Return the car number: ncarnum */
    int getCarNum() { return ncarnum; }

    /** Return the IP Address: ipaddr */
    std::string getIPAddr() { return ipaddr; }

    /**Return the certificate: cert */
    gnutls_datum_t getCertificate() { return cert; }

    /** Return the number of seats in the car: ncarsize */
    int getCarSize() { return ncarsize; }

    /** Get the file descriptor for the socket */
    int getFD() { return fdAccepted; }

    /** Is the client verified? */
    int isVerified()
    {
      return verified ? true : false;
    }

    /* Mutators */

    /** Set ncarnum */
    int setCarNum(int in_ncarnum) 
    { 
      ncarnum = in_ncarnum; 
      return 0;
    }

    /** Set ipaddr */
    int setIPAddr(std::string in_ipaddr)
    {
      ipaddr = in_ipaddr; 
      return 0;
    }

    /** Set cert */
    int setCertificate(gnutls_datum_t  in_cert)
    {
      cert = in_cert;
      return 0;
    }

    /** Set Auth Code */
    int setAuthCode(std::string in_authcode)
    {
      authcode = in_authcode;
      return 0;
    }

    /** Set both NetIDs */
    int setNetIDs(std::string driver, std::string ridealong)
    {
      netidDriver = driver;
      netidRideAlong = ridealong;
      return 0;
    }

    /** Set the number of seats in the car */
    int setCarSize(int seats)
    {
      ncarsize = seats;
      return 0;
    }

    /**Set fdAccepted */
    int setFD(int in_fdAccepted)
    {
      fdAccepted = in_fdAccepted;
      return 0;
    }

    /** Set client is verified */
    int setVerified(int bit)
    {
      verified = bit;
      return 0;
    }
};
#endif  // gtbcommunication_h
