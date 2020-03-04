﻿using System;
using System.Collections.Generic;
using System.Data.OleDb;
using Microsoft.Search.Interop;

namespace Wox.Plugin.WindowsIndexer.SearchHelper
{
    public class WindowsSearchAPI
    {
        public IEnumerable<SearchResult> Search(string keyword, string pattern = "*", int offset = 0, int maxCount = 100)
        {
            // This uses the Microsoft.Search.Interop assembly
            CSearchManager manager = new CSearchManager();

            // SystemIndex catalog is the default catalog in Windows
            ISearchCatalogManager catalogManager = manager.GetCatalog("SystemIndex");

            // Get the ISearchQueryHelper which will help us to translate AQS --> SQL necessary to query the indexer
            ISearchQueryHelper queryHelper = catalogManager.GetQueryHelper();

            // Set the number of results we want. Don't set this property if all results are needed.
            queryHelper.QueryMaxResults = maxCount;

            // Set list of columns we want to display, getting the path presently
            queryHelper.QuerySelectColumns = "System.ItemPathDisplay";

            // Set additional query restriction
            queryHelper.QueryWhereRestrictions = "AND scope='file:'";

            // convert file pattern if it is not '*'. Don't create restriction for '*' as it includes all files.
            if (pattern != "*")
            {
                pattern = pattern.Replace("*", "%");
                pattern = pattern.Replace("?", "_");

                if (pattern.Contains("%") || pattern.Contains("_"))
                {
                    queryHelper.QueryWhereRestrictions += " AND System.FileName LIKE '" + pattern + "' ";
                }
                else
                {
                    // if there are no wildcards we can use a contains which is much faster as it uses the index
                    queryHelper.QueryWhereRestrictions += " AND Contains(System.FileName, '" + pattern + "') ";
                }
            }

            // Set sorting order 
            queryHelper.QuerySorting = "System.DateModified DESC";

            // Generate SQL from our parameters, converting the userQuery from AQS->WHERE clause
            string sqlQuery = queryHelper.GenerateSQLFromUserQuery(keyword);

            // --- Perform the query ---
            // create an OleDbConnection object which connects to the indexer provider with the windows application
            using (System.Data.OleDb.OleDbConnection conn = new OleDbConnection(queryHelper.ConnectionString))
            {
                // open the connection
                conn.Open();

                // now create an OleDB command object with the query we built above and the connection we just opened.
                using (OleDbCommand command = new OleDbCommand(sqlQuery, conn))
                {
                    // execute the command, which returns the results as an OleDbDataReader.
                    using (OleDbDataReader WDSResults = command.ExecuteReader())
                    {
                        while (WDSResults.Read())
                        {
                            // col 0 is our path in display format
                            Console.WriteLine("{0}", WDSResults.GetString(0));
                            var result = new SearchResult { Path = WDSResults.GetString(0) };

                            yield return result;
                        }
                    }

                }
            }

        }
    }
}
