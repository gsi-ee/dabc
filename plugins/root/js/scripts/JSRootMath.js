/// @file JSRootMath.js
/// Special mathematical functions

(function( factory ) {
   if ( typeof define === "function" && define.amd ) {
      // AMD. Register as an anonymous module.
      define( ['JSRootCore'], factory );
   } else {
      if (typeof JSROOT == 'undefined')
         throw new Error("This extension requires JSRootCore.js", "JSRootMath.js");

      if (typeof JSROOT.Math == "object")
         throw new Error("This JSROOT Math already loaded", "JSRootMath.js");

      factory(JSROOT);
   }
} (function(JSROOT) {
   // math methods for Javascript ROOT

   JSROOT.Math = {};

   JSROOT.Math.lgam = function( x ) {
      var p, q, u, w, z, i, sgngam = 1;
      var kMAXLGM  = 2.556348e305;
      var LS2PI  =  0.91893853320467274178;

      var A = [
         8.11614167470508450300E-4,
         -5.95061904284301438324E-4,
         7.93650340457716943945E-4,
         -2.77777777730099687205E-3,
         8.33333333333331927722E-2
      ];

      var B = [
         -1.37825152569120859100E3,
         -3.88016315134637840924E4,
         -3.31612992738871184744E5,
         -1.16237097492762307383E6,
         -1.72173700820839662146E6,
         -8.53555664245765465627E5
      ];

      var C = [
      /* 1.00000000000000000000E0, */
         -3.51815701436523470549E2,
         -1.70642106651881159223E4,
         -2.20528590553854454839E5,
         -1.13933444367982507207E6,
         -2.53252307177582951285E6,
         -2.01889141433532773231E6
      ];

      if (x >= Number.POSITIVE_INFINITY)
         return(Number.POSITIVE_INFINITY);

      if ( x < -34.0 ) {
         q = -x;
         w = this.lgam(q);
         p = Math.floor(q);
         if ( p==q )//_unur_FP_same(p,q)
            return (Number.POSITIVE_INFINITY);
         i = Math.round(p);
         if ( (i & 1) == 0 )
            sgngam = -1;
         else
            sgngam = 1;
         z = q - p;
         if ( z > 0.5 ) {
            p += 1.0;
            z = p - q;
         }
         z = q * Math.sin( Math.PI * z );
         if ( z < 1e-300 )
            return (Number.POSITIVE_INFINITY);
         z = Math.log(Math.PI) - Math.log( z ) - w;
         return( z );
      }
      if ( x < 13.0 ) {
         z = 1.0;
         p = 0.0;
         u = x;
         while ( u >= 3.0 ) {
            p -= 1.0;
            u = x + p;
            z *= u;
         }
         while ( u < 2.0 ) {
            if ( u < 1e-300 )
               return (Number.POSITIVE_INFINITY);
            z /= u;
            p += 1.0;
            u = x + p;
         }
         if ( z < 0.0 ) {
            sgngam = -1;
            z = -z;
         }
         else
            sgngam = 1;
         if ( u == 2.0 )
            return( Math.log(z) );
         p -= 2.0;
         x = x + p;
         p = x * this.Polynomialeval(x, B, 5 ) / this.Polynomial1eval( x, C, 6);
         return( Math.log(z) + p );
      }
      if ( x > kMAXLGM )
         return( sgngam * Number.POSITIVE_INFINITY );

      q = ( x - 0.5 ) * Math.log(x) - x + LS2PI;
      if ( x > 1.0e8 )
         return( q );

      p = 1.0/(x*x);
      if ( x >= 1000.0 )
         q += ((7.9365079365079365079365e-4 * p
               - 2.7777777777777777777778e-3) *p
               + 0.0833333333333333333333) / x;
      else
         q += this.Polynomialeval( p, A, 4 ) / x;
      return( q );
   };

   /*
    * calculates a value of a polynomial of the form:
    * a[0]x^N+a[1]x^(N-1) + ... + a[N]
   */
   JSROOT.Math.Polynomialeval = function(x, a, N) {
      if (N==0) return a[0];
      else {
         var pom = a[0];
         for (var i=1; i <= N; ++i)
            pom = pom *x + a[i];
         return pom;
      }
   };

   /*
    * calculates a value of a polynomial of the form:
    * x^N+a[0]x^(N-1) + ... + a[N-1]
   */
   JSROOT.Math.Polynomial1eval = function(x, a, N) {
      if (N==0) return a[0];
      else {
         var pom = x + a[0];
         for (var i=1; i < N; ++i)
            pom = pom *x + a[i];
         return pom;
      }
   };

   JSROOT.Math.ndtri = function( y0 ) {
      if ( y0 <= 0.0 )
         return( Number.NEGATIVE_INFINITY );
      if ( y0 >= 1.0 )
         return( Number.POSITIVE_INFINITY );

      var P0 = new Array(
           -5.99633501014107895267E1,
            9.80010754185999661536E1,
           -5.66762857469070293439E1,
            1.39312609387279679503E1,
           -1.23916583867381258016E0
      );

      var Q0 = new Array(
            1.95448858338141759834E0,
            4.67627912898881538453E0,
            8.63602421390890590575E1,
           -2.25462687854119370527E2,
            2.00260212380060660359E2,
           -8.20372256168333339912E1,
            1.59056225126211695515E1,
           -1.18331621121330003142E0
      );

      var P1 = new Array(
            4.05544892305962419923E0,
            3.15251094599893866154E1,
            5.71628192246421288162E1,
            4.40805073893200834700E1,
            1.46849561928858024014E1,
            2.18663306850790267539E0,
           -1.40256079171354495875E-1,
           -3.50424626827848203418E-2,
           -8.57456785154685413611E-4
      );

      var Q1 = new Array(
            1.57799883256466749731E1,
            4.53907635128879210584E1,
            4.13172038254672030440E1,
            1.50425385692907503408E1,
            2.50464946208309415979E0,
           -1.42182922854787788574E-1,
           -3.80806407691578277194E-2,
           -9.33259480895457427372E-4
      );

      var P2 = new Array(
            3.23774891776946035970E0,
            6.91522889068984211695E0,
            3.93881025292474443415E0,
            1.33303460815807542389E0,
            2.01485389549179081538E-1,
            1.23716634817820021358E-2,
            3.01581553508235416007E-4,
            2.65806974686737550832E-6,
            6.23974539184983293730E-9
      );

      var Q2 = new Array(
            6.02427039364742014255E0,
            3.67983563856160859403E0,
            1.37702099489081330271E0,
            2.16236993594496635890E-1,
            1.34204006088543189037E-2,
            3.28014464682127739104E-4,
            2.89247864745380683936E-6,
            6.79019408009981274425E-9
      );

      var s2pi = 2.50662827463100050242e0;
      var code = 1;
      var y = y0;
      var x, z, y2, x0, x1;

      if ( y > (1.0 - 0.13533528323661269189) ) {
         y = 1.0 - y;
         code = 0;
      }
      if ( y > 0.13533528323661269189 ) {
         y = y - 0.5;
         y2 = y * y;
         x = y + y * (y2 * this.Polynomialeval( y2, P0, 4)/ this.Polynomial1eval( y2, Q0, 8 ));
         x = x * s2pi;
         return(x);
      }
      x = Math.sqrt( -2.0 * Math.log(y) );
      x0 = x - Math.log(x)/x;
      z = 1.0/x;
      if ( x < 8.0 )
         x1 = z * this.Polynomialeval( z, P1, 8 )/ this.Polynomial1eval( z, Q1, 8 );
      else
         x1 = z * this.Polynomialeval( z, P2, 8 )/ this.Polynomial1eval( z, Q2, 8 );
      x = x0 - x1;
      if ( code != 0 )
         x = -x;
      return( x );
   }

   JSROOT.Math.igam = function(a,x) {
      var kMACHEP = 1.11022302462515654042363166809e-16;
      var kMAXLOG = 709.782712893383973096206318587;
      var ans, ax, c, r;

      // LM: for negative values returns 1.0 instead of zero
      // This is correct if a is a negative integer since Gamma(-n) = +/- inf
      if (a <= 0)  return 1.0;

      if (x <= 0)  return 0.0;

      if( (x > 1.0) && (x > a ) )
         return( 1.0 - this.igamc(a,x) );

      /* Compute  x**a * exp(-x) / gamma(a)  */
      ax = a * Math.log(x) - x - this.lgam(a);
      if( ax < -kMAXLOG )
         return( 0.0 );

      ax = Math.exp(ax);

      /* power series */
      r = a;
      c = 1.0;
      ans = 1.0;

      do
      {
         r += 1.0;
         c *= x/r;
         ans += c;
      }
      while( c/ans > kMACHEP );

      return( ans * ax/a );
   }

   JSROOT.Math.igamc = function(a,x) {
      var kMACHEP = 1.11022302462515654042363166809e-16;
      var kMAXLOG = 709.782712893383973096206318587;

      var kBig = 4.503599627370496e15;
      var kBiginv =  2.22044604925031308085e-16;

      var ans, ax, c, yc, r, t, y, z;
      var pk, pkm1, pkm2, qk, qkm1, qkm2;

      // LM: for negative values returns 0.0
      // This is correct if a is a negative integer since Gamma(-n) = +/- inf
      if (a <= 0)  return 0.0;

      if (x <= 0) return 1.0;

      if( (x < 1.0) || (x < a) )
         return ( 1.0 - JSROOT.Math.igam(a,x) );

      ax = a * Math.log(x) - x - JSROOT.Math.lgam(a);
      if( ax < -kMAXLOG )
         return( 0.0 );

      ax = Math.exp(ax);

      /* continued fraction */
      y = 1.0 - a;
      z = x + y + 1.0;
      c = 0.0;
      pkm2 = 1.0;
      qkm2 = x;
      pkm1 = x + 1.0;
      qkm1 = z * x;
      ans = pkm1/qkm1;

      do
      {
         c += 1.0;
         y += 1.0;
         z += 2.0;
         yc = y * c;
         pk = pkm1 * z  -  pkm2 * yc;
         qk = qkm1 * z  -  qkm2 * yc;
         if(qk)
         {
            r = pk/qk;
            t = Math.abs( (ans - r)/r );
            ans = r;
         }
         else
            t = 1.0;
         pkm2 = pkm1;
         pkm1 = pk;
         qkm2 = qkm1;
         qkm1 = qk;
         if( Math.abs(pk) > kBig )
         {
            pkm2 *= kBiginv;
            pkm1 *= kBiginv;
            qkm2 *= kBiginv;
            qkm1 *= kBiginv;
         }
      }
      while( t > kMACHEP );

      return( ans * ax );
   }


   JSROOT.Math.igami = function(a, y0) {
      var x0, x1, x, yl, yh, y, d, lgm, dithresh;
      var i, dir;
      var kMACHEP = 1.11022302462515654042363166809e-16;

      // check the domain
      if (a <= 0) {
         alert("igami : Wrong domain for parameter a (must be > 0)");
         return 0;
      }
      if (y0 <= 0) {
         return Number.POSITIVE_INFINITY;
      }
      if (y0 >= 1) {
         return 0;
      }
      /* bound the solution */
      var kMAXNUM = Number.MAX_VALUE;
      x0 = kMAXNUM;
      yl = 0;
      x1 = 0;
      yh = 1.0;
      dithresh = 5.0 * kMACHEP;

      /* approximation to inverse function */
      d = 1.0/(9.0*a);
      y = ( 1.0 - d - this.ndtri(y0) * Math.sqrt(d) );
      x = a * y * y * y;

      lgm = this.lgam(a);

      for( i=0; i<10; ++i ) {
         if ( x > x0 || x < x1 )
            break;
         y = this.igamc(a,x);
         if ( y < yl || y > yh )
            break;
         if ( y < y0 ) {
            x0 = x;
            yl = y;
         }
         else {
            x1 = x;
            yh = y;
         }
         /* compute the derivative of the function at this point */
         d = (a - 1.0) * Math.log(x) - x - lgm;
         if ( d < -kMAXLOG )
            break;
         d = -Math.exp(d);
         /* compute the step to the next approximation of x */
         d = (y - y0)/d;
         if ( Math.abs(d/x) < kMACHEP )
            return( x );
         x = x - d;
      }
      /* Resort to interval halving if Newton iteration did not converge. */
      d = 0.0625;
      if ( x0 == kMAXNUM ) {
         if ( x <= 0.0 )
            x = 1.0;
         while ( x0 == kMAXNUM ) {
            x = (1.0 + d) * x;
            y = this.igamc( a, x );
            if ( y < y0 ) {
               x0 = x;
               yl = y;
               break;
            }
            d = d + d;
         }
      }
      d = 0.5;
      dir = 0;

      for( i=0; i<400; ++i ) {
         x = x1  +  d * (x0 - x1);
         y = this.igamc( a, x );
         lgm = (x0 - x1)/(x1 + x0);
         if ( Math.abs(lgm) < dithresh )
            break;
         lgm = (y - y0)/y0;
         if ( Math.abs(lgm) < dithresh )
            break;
         if ( x <= 0.0 )
            break;
         if ( y >= y0 ) {
            x1 = x;
            yh = y;
            if ( dir < 0 ) {
               dir = 0;
               d = 0.5;
            }
            else if ( dir > 1 )
               d = 0.5 * d + 0.5;
            else
               d = (y0 - yl)/(yh - yl);
            dir += 1;
         }
         else {
            x0 = x;
            yl = y;
            if ( dir > 0 ) {
               dir = 0;
               d = 0.5;
            }
            else if ( dir < -1 )
               d = 0.5 * d;
            else
               d = (y0 - yl)/(yh - yl);
            dir -= 1;
         }
      }
      return( x );
   };

   JSROOT.Math.gamma_quantile_c = function(z, alpha, theta) {
      return theta * this.igami( alpha, z);
   };

   JSROOT.Math.gamma_quantile = function(z, alpha, theta) {
      return theta * this.igami( alpha, 1.- z);
   };

   JSROOT.Math.log10 = function(n) {
      return Math.log(n) / Math.log(10);
   };

   JSROOT.Math.landau_pdf = function(x, xi, x0) {
      // LANDAU pdf : algorithm from CERNLIB G110 denlan
      // same algorithm is used in GSL
      if (xi <= 0) return 0;
      var v = (x - x0)/xi;
      var u, ue, us, denlan;
      var p1 = new Array(0.4259894875,-0.1249762550, 0.03984243700, -0.006298287635,   0.001511162253);
      var q1 = new Array(1.0         ,-0.3388260629, 0.09594393323, -0.01608042283,    0.003778942063);
      var p2 = new Array(0.1788541609, 0.1173957403, 0.01488850518, -0.001394989411,   0.0001283617211);
      var q2 = new Array(1.0         , 0.7428795082, 0.3153932961,   0.06694219548,    0.008790609714);
      var p3 = new Array(0.1788544503, 0.09359161662,0.006325387654, 0.00006611667319,-0.000002031049101);
      var q3 = new Array(1.0         , 0.6097809921, 0.2560616665,   0.04746722384,    0.006957301675);
      var p4 = new Array(0.9874054407, 118.6723273,  849.2794360,   -743.7792444,      427.0262186);
      var q4 = new Array(1.0         , 106.8615961,  337.6496214,    2016.712389,      1597.063511);
      var p5 = new Array(1.003675074,  167.5702434,  4789.711289,    21217.86767,     -22324.94910);
      var q5 = new Array(1.0         , 156.9424537,  3745.310488,    9834.698876,      66924.28357);
      var p6 = new Array(1.000827619,  664.9143136,  62972.92665,    475554.6998,     -5743609.109);
      var q6 = new Array(1.0         , 651.4101098,  56974.73333,    165917.4725,     -2815759.939);
      var a1 = new Array(0.04166666667,-0.01996527778, 0.02709538966);
      var a2 = new Array(-1.845568670,-4.284640743);

      if (v < -5.5) {
         u   = Math.exp(v+1.0);
         if (u < 1e-10) return 0.0;
         ue  = Math.exp(-1/u);
         us  = Math.sqrt(u);
         denlan = 0.3989422803*(ue/us)*(1+(a1[0]+(a1[1]+a1[2]*u)*u)*u);
      } else if(v < -1) {
         u   = Math.exp(-v-1);
         denlan = Math.exp(-u)*Math.sqrt(u)*
            (p1[0]+(p1[1]+(p1[2]+(p1[3]+p1[4]*v)*v)*v)*v)/
            (q1[0]+(q1[1]+(q1[2]+(q1[3]+q1[4]*v)*v)*v)*v);
      } else if(v < 1) {
         denlan = (p2[0]+(p2[1]+(p2[2]+(p2[3]+p2[4]*v)*v)*v)*v)/
            (q2[0]+(q2[1]+(q2[2]+(q2[3]+q2[4]*v)*v)*v)*v);
      } else if(v < 5) {
         denlan = (p3[0]+(p3[1]+(p3[2]+(p3[3]+p3[4]*v)*v)*v)*v)/
            (q3[0]+(q3[1]+(q3[2]+(q3[3]+q3[4]*v)*v)*v)*v);
      } else if(v < 12) {
         u   = 1/v;
         denlan = u*u*(p4[0]+(p4[1]+(p4[2]+(p4[3]+p4[4]*u)*u)*u)*u)/
            (q4[0]+(q4[1]+(q4[2]+(q4[3]+q4[4]*u)*u)*u)*u);
      } else if(v < 50) {
         u   = 1/v;
         denlan = u*u*(p5[0]+(p5[1]+(p5[2]+(p5[3]+p5[4]*u)*u)*u)*u)/
            (q5[0]+(q5[1]+(q5[2]+(q5[3]+q5[4]*u)*u)*u)*u);
      } else if(v < 300) {
         u   = 1/v;
         denlan = u*u*(p6[0]+(p6[1]+(p6[2]+(p6[3]+p6[4]*u)*u)*u)*u)/
            (q6[0]+(q6[1]+(q6[2]+(q6[3]+q6[4]*u)*u)*u)*u);
      } else {
         u   = 1/(v-v*Math.log(v)/(v+1));
         denlan = u*u*(1+(a2[0]+a2[1]*u)*u);
      }
      return denlan/xi;
   };

   JSROOT.Math.Landau = function(x, mpv, sigma, norm) {
      if (sigma <= 0) return 0;
      var den = JSROOT.Math.landau_pdf((x - mpv) / sigma, 1, 0);
      if (!norm) return den;
      return den/sigma;
   }

   JSROOT.Math.inc_gamma_c = function(a,x) {
      return JSROOT.Math.igamc(a,x);
   }

   JSROOT.Math.chisquared_cdf_c = function(x,r,x0) {
     return JSROOT.Math.inc_gamma_c ( 0.5 * r , 0.5* (x-x0) );
   }

   JSROOT.Math.Prob = function(chi2, ndf) {
      if (ndf <= 0) return 0; // Set CL to zero in case ndf<=0

      if (chi2 <= 0) {
         if (chi2 < 0) return 0;
         else          return 1;
      }

      return JSROOT.Math.chisquared_cdf_c(chi2,ndf,0);
   }

   JSROOT.Math.gaus = function(f, x, i) {
      return f['fParams'][i+0] * Math.exp(-0.5 * Math.pow((x-f['fParams'][i+1]) / f['fParams'][i+2], 2));
   };

   JSROOT.Math.gausn = function(f, x, i) {
      return JSROOT.Math.gaus(f, x, i)/(Math.sqrt(2 * Math.PI) * f['fParams'][i+2]);
   };

   JSROOT.Math.expo = function(f, x, i) {
      return Math.exp(f['fParams'][i+0] + f['fParams'][i+1] * x);
   };

   JSROOT.Math.landau = function(f, x, i) {
      return JSROOT.Math.Landau(x, f['fParams'][i+1],f['fParams'][i+2], false);
   }

   JSROOT.Math.landaun = function(f, x, i) {
      return JSROOT.Math.Landau(x, f['fParams'][i+1],f['fParams'][i+2], true);
   }

   return JSROOT;

}));
