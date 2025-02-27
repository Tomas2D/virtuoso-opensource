/*
 *  $Id$
 *
 *  This file is part of the OpenLink Software Virtuoso Open-Source (VOS)
 *  project.
 *
 *  Copyright (C) 1998-2022 OpenLink Software
 *
 *  This project is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; only version 2 of the License, dated June 1991.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

import java.io.*;
import java.net.URL;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.util.*;

import org.eclipse.rdf4j.model.*;
import org.eclipse.rdf4j.query.BindingSet;
import org.eclipse.rdf4j.query.BooleanQuery;
import org.eclipse.rdf4j.query.GraphQuery;
import org.eclipse.rdf4j.query.GraphQueryResult;
import org.eclipse.rdf4j.query.MalformedQueryException;
import org.eclipse.rdf4j.query.QueryEvaluationException;
import org.eclipse.rdf4j.query.QueryLanguage;
import org.eclipse.rdf4j.query.TupleQuery;
import org.eclipse.rdf4j.query.TupleQueryResult;
import org.eclipse.rdf4j.repository.Repository;
import org.eclipse.rdf4j.repository.RepositoryConnection;
import org.eclipse.rdf4j.repository.RepositoryException;
import org.eclipse.rdf4j.repository.RepositoryResult;
import org.eclipse.rdf4j.rio.RDFFormat;
import org.eclipse.rdf4j.rio.RDFHandler;
import org.eclipse.rdf4j.rio.ntriples.NTriplesWriter;

import virtuoso.rdf4j.driver.*;

public class Test_BNode_perf1 {

    public static final String VIRTUOSO_INSTANCE = "localhost";
    public static final int VIRTUOSO_PORT = 1111;
    public static final String VIRTUOSO_USERNAME = "dba";
    public static final String VIRTUOSO_PASSWORD = "dba";


    public static void log(String mess) {
        System.out.println("   " + mess);
    }


    public static void main(String[] args) {

        Perf_ImportFromFile(args, false);
        Perf_ImportFromFile(args, true);
    }



    public static void Perf_ImportFromFile(String[] args, boolean insertBNodeAsIRI) {

        String[] sa = new String[4];
        sa[0] = VIRTUOSO_INSTANCE;
        sa[1] = VIRTUOSO_PORT + "";
        sa[2] = VIRTUOSO_USERNAME;
        sa[3] = VIRTUOSO_PASSWORD;
        for (int i = 0; i < sa.length && i < args.length; i++) {
            sa[i] = args[i];
        }
        VirtuosoRepository repository = new VirtuosoRepository("jdbc:virtuoso://" + sa[0] + ":" + sa[1], sa[2], sa[3]);
        repository.setInsertBNodeAsVirtuosoIRI(insertBNodeAsIRI);

        RepositoryConnection con = null;
        try {
            con = repository.getConnection();
            con.setAutoCommit(true);

            // test add data to the repository
            boolean ok = true;
            String query = null;
            RepositoryResult<Statement> rs;
            Statement st;
            ValueFactory vfac = repository.getValueFactory();
            IRI context = vfac.createIRI("test:blank");

            System.out.println("\n\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
            if (insertBNodeAsIRI)
              System.out.println("Test Import data from File (BNode as Virtuoso IRI)");
            else
              System.out.println("Test Import data from File (BNode as Virtuoso Native BNode)");
            System.out.println("++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
            log("Insert data with BNodes from file sp2b.n3");


            int REPEAT=3;
            long cum_time=0;

            for(int i=0; i<REPEAT; i++) {

              con.clear(context);

              IRI ns = vfac.createIRI("http://localhost/publications/journals/Journal3/1967");
              IRI np = vfac.createIRI("http://swrc.ontoware.org/ontology#editor");
              IRI np1 = vfac.createIRI("http://xmlns.com/foaf/0.1/name");

              log("== Exec "+i);
              try
              {
	        File dataFile = new File("sp2b.n3");
	        if (!con.isActive())
	           con.begin();
	        long start_time = System.currentTimeMillis();
	        con.add(dataFile, "", RDFFormat.N3, context);
//	        con.add(new BufferedReader(new FileReader("sp2b.n3")), "", RDFFormat.N3, context);
	        long end_time = System.currentTimeMillis(); 
   	        con.commit();
	        long tst_time = (end_time-start_time);
	        cum_time += tst_time;
 	        log("Time :"+(end_time-start_time)+" ms");

   	        long count = con.size(context);
 	        log("Inserted :"+count+" triples");

              } catch (Exception e) {
                log("***FAILED Test "+e);
                ok = false;
              }
            }
            log("AVG TIME = "+cum_time/REPEAT+" ms");

        }
        catch (Exception e) {
            System.out.println("ERROR Test Failed.");
            e.printStackTrace();
        }
        finally {
            if (con != null) try {
                con.close();
            }
            catch (RepositoryException e) {
                e.printStackTrace();
            }
        }
    }

}
