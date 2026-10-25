include_directories(
${CMAKE_SOURCE_DIR}
${CMAKE_SOURCE_DIR}/scribus
)

# Gentoo has an option for no templates. Let's make the ebuild simpler and open up the option to others, but leave as a default for us. Make the directory in any case as our source will look for it.
if (WANT_NOTEMPLATES)
  install(CODE "file(MAKE_DIRECTORY ${ENV}${CMAKE_INSTALL_PREFIX}/${TEMPLATEDIR})")
else (WANT_NOTEMPLATES)

install(FILES
template.xml
template.ca.xml
template.de.xml
template.es.xml
template.hu.xml
template.it.xml
  DESTINATION ${TEMPLATEDIR}
)

install(FILES
br1/br1.png
br1/br1tn.png
br1/map.jpg
br1/br1.sla.gz
  DESTINATION ${TEMPLATEDIR}br1
)

install(FILES
nl1/kites.jpg
nl1/nl1.png
nl1/nl1tn.png
nl1/nl1.sla.gz
  DESTINATION ${TEMPLATEDIR}nl1
)

install(FILES
nl2/flowers.jpg
nl2/nl2.png
nl2/nl2tn.png
nl2/nl2.sla.gz
  DESTINATION ${TEMPLATEDIR}nl2
)

install(FILES
textbased/an_image.jpg
textbased/author.jpg
textbased/textbased.png
textbased/textbased.sla.gz
textbased/textbasedtn.png
  DESTINATION ${TEMPLATEDIR}textbased
)

install(FILES
pres_backgr_1/blue_orange_vector.sla.gz
pres_backgr_1/blue_orange_vectortn.png
pres_backgr_1/blue_orange_vector.png
  DESTINATION ${TEMPLATEDIR}pres_backgr_1
)

install(FILES
pres_backgr_2/flood_light.sla.gz
pres_backgr_2/flood_lighttn.png
pres_backgr_2/flood_light.png
  DESTINATION ${TEMPLATEDIR}pres_backgr_2
)

install(FILES
pres_backgr_3/glass_ripples.sla.gz
pres_backgr_3/glass_ripplestn.png
pres_backgr_3/glass_ripples.png
  DESTINATION ${TEMPLATEDIR}pres_backgr_3
)

install(FILES
pres_backgr_4/bokeh.sla.gz
pres_backgr_4/Bokehtn.png
pres_backgr_4/Bokeh.png
  DESTINATION ${TEMPLATEDIR}pres_backgr_4
)

install(FILES
cc/Cheat_Cube.png
cc/Cheat_Cubetn.png
cc/Cheat_Cube.sla.gz
  DESTINATION ${TEMPLATEDIR}cc
)

install(FILES
mc/menu_template1.png
mc/menu_template1tn.png
mc/menu_template1.sla.gz
  DESTINATION ${TEMPLATEDIR}mc
)

install(FILES
cover1/cover1.png
cover1/cover1tn.png
cover1/cover1.sla.gz
  DESTINATION ${TEMPLATEDIR}cover1
)

install(FILES
cover2/cover2.png
cover2/cover2tn.png
cover2/cover2.sla.gz
  DESTINATION ${TEMPLATEDIR}cover2
)

install(FILES
cover3/cover3.png
cover3/cover3tn.png
cover3/cover3.sla.gz
  DESTINATION ${TEMPLATEDIR}cover3
)

install(FILES
cover4/cover4.png
cover4/cover4tn.png
cover4/cover4.sla.gz
  DESTINATION ${TEMPLATEDIR}cover4
)

install(FILES
cover5/cover5.png
cover5/cover5tn.png
cover5/cover5.sla.gz
  DESTINATION ${TEMPLATEDIR}cover5
)

install(FILES
news_gn_notice/greater_noida_flat.png
news_gn_notice/news_gn_notice.png
news_gn_notice/news_gn_noticetn.png
news_gn_notice/news_gn_notice.sla.gz
  DESTINATION ${TEMPLATEDIR}news_gn_notice
)

install(FILES
news_court_notice_3x10/news_court_notice_3x10.png
news_court_notice_3x10/news_court_notice_3x10tn.png
news_court_notice_3x10/news_court_notice_3x10.sla.gz
  DESTINATION ${TEMPLATEDIR}news_court_notice_3x10
)

install(FILES
buscard1/Business_Card_1.png
buscard1/Business_Card_1tn.png
buscard1/buscard1.sla.gz
  DESTINATION ${TEMPLATEDIR}buscard1
)

install(FILES
buscard2/Business_Card_2.png
buscard2/Business_Card_2tn.png
buscard2/buscard2.sla.gz
  DESTINATION ${TEMPLATEDIR}buscard2
)

install(FILES
buscard3/Business_Card_3.png
buscard3/Business_Card_3tn.png
buscard3/buscard3.sla.gz
  DESTINATION ${TEMPLATEDIR}buscard3
)

install(FILES
buscard4/Business_Card_4.png
buscard4/Business_Card_4tn.png
buscard4/buscard4.sla.gz
  DESTINATION ${TEMPLATEDIR}buscard4
)

install(FILES
buscard5/Business_Card_5.png
buscard5/Business_Card_5tn.png
buscard5/buscard5.sla.gz
  DESTINATION ${TEMPLATEDIR}buscard5
)

install(FILES
buscard6/Business_Card_6.png
buscard6/Business_Card_6tn.png
buscard6/buscard6.sla.gz
  DESTINATION ${TEMPLATEDIR}buscard6
)

install(FILES
buscard7/Business_Card_7.png
buscard7/Business_Card_7tn.png
buscard7/buscard7.sla.gz
  DESTINATION ${TEMPLATEDIR}buscard7
)

install(FILES
buscard8/Business_Card_8.png
buscard8/Business_Card_8tn.png
buscard8/buscard8.sla.gz
  DESTINATION ${TEMPLATEDIR}buscard8
)

install(FILES
buscard9/Business_Card_9.png
buscard9/Business_Card_9tn.png
buscard9/buscard9.sla.gz
  DESTINATION ${TEMPLATEDIR}buscard9
)

install(FILES
buscard10/Business_Card_10.png
buscard10/Business_Card_10tn.png
buscard10/buscard10.sla.gz
  DESTINATION ${TEMPLATEDIR}buscard10
)

install(FILES
buscard11/Business_Card_11.png
buscard11/Business_Card_11tn.png
buscard11/buscard11.sla.gz
  DESTINATION ${TEMPLATEDIR}buscard11
)

install(FILES
grid_br_a4_l/Brochure_Grid_A4_L.sla.gz
grid_br_a4_l/Brochure_Grid_A4_Ltn.png
grid_br_a4_l/Brochure_Grid_A4_L.png
  DESTINATION ${TEMPLATEDIR}grid_br_a4_l
)

install(FILES
grid_br_a4_p/Brochure_Grid_A4_P.sla.gz
grid_br_a4_p/Brochure_Grid_A4_Ptn.png
grid_br_a4_p/Brochure_Grid_A4_P.png
  DESTINATION ${TEMPLATEDIR}grid_br_a4_p
)

install(FILES
grid_br_975x85in_l/Brochure_Grid_975x85in_L.sla.gz
grid_br_975x85in_l/Brochure_Grid_975x85_in_Ltn.png
grid_br_975x85in_l/Brochure_Grid_975x85_in_L.png
  DESTINATION ${TEMPLATEDIR}grid_br_975x85in_l
)

install(FILES
grid_br_975x85_p/Brochure_Grid_975x85in_P.sla.gz
grid_br_975x85_p/Brochure_Grid_975x85_in_Ptn.png
grid_br_975x85_p/Brochure_Grid_975x85_in_P.png
  DESTINATION ${TEMPLATEDIR}grid_br_975x85_p
)

install(FILES
grid_br_375x85_p/Brochure_Grid_375x85in_P.sla.gz
grid_br_375x85_p/Brochure_Grid_375x85_in_Ptn.png
grid_br_375x85_p/Brochure_Grid_375x85_in_P.png
  DESTINATION ${TEMPLATEDIR}grid_br_375x85_p
)

install(FILES
grid_br_375x85_l/Brochure_Grid_375x85in_L.sla.gz
grid_br_375x85_l/Brochure_Grid_375x85_in_Landscapetn.png
grid_br_375x85_l/Brochure_Grid_375x85_in_Landscape.png
  DESTINATION ${TEMPLATEDIR}grid_br_375x85_l
)

install(FILES
grid_br_100x210_p/Brochure_Grid_100x210_P.sla.gz
grid_br_100x210_p/Brochure_Grid_100x210_Ptn.png
grid_br_100x210_p/Brochure_Grid_100x210_P.png
  DESTINATION ${TEMPLATEDIR}grid_br_100x210_p
)

install(FILES
grid_br_100x210_l/Brochure_Grid_100x210_L.sla.gz
grid_br_100x210_l/Brochure_Grid_100x210_mm_Landscapetn.png
grid_br_100x210_l/Brochure_Grid_100x210_mm_Landscape.png
  DESTINATION ${TEMPLATEDIR}grid_br_100x210_l
)

install(FILES
grid_br_55x86in_p/Brochure_Grid_55x85in_P.sla.gz
grid_br_55x86in_p/Brochure_Grid_55x85in_Portraoittn.png
grid_br_55x86in_p/Brochure_Grid_55x85in_Portraoit.png
  DESTINATION ${TEMPLATEDIR}grid_br_55x86in_p
)

install(FILES
grid_br_55x85in_l/Brochure_Grid_55x85in_L.sla.gz
grid_br_55x85in_l/Brochure_Grid_55x85_in_Ltn.png
grid_br_55x85in_l/Brochure_Grid_55x85_in_L.png
  DESTINATION ${TEMPLATEDIR}grid_br_55x85in_l
)

install(FILES
grid_poster_a3/Poster_Grid_A3.sla.gz
grid_poster_a3/Poster_Grid_A3tn.png
grid_poster_a3/Poster_Grid_A3.png
  DESTINATION ${TEMPLATEDIR}grid_poster_a3
)

install(FILES
grid_poster_a2/Poster_Grid_A2.sla.gz
grid_poster_a2/Poster_Grid_A2tn.png
grid_poster_a2/Poster_Grid_A2.png
  DESTINATION ${TEMPLATEDIR}grid_poster_a2
)

install(FILES
grid_poster_a1/Poster_Grid_A1.sla.gz
grid_poster_a1/Poster_Grid_A1tn.png
grid_poster_a1/Poster_Grid_A1.png
  DESTINATION ${TEMPLATEDIR}grid_poster_a1
)

install(FILES
grid_poster_a0/Poster_Grid_A0.sla.gz
grid_poster_a0/Poster_Grid_A0tn.png
grid_poster_a0/Poster_Grid_A0.png
  DESTINATION ${TEMPLATEDIR}grid_poster_a0
)

install(FILES
grid_br_usl_p/Brochure_Grid_USL_P.sla.gz
grid_br_usl_p/Brochure_Grid_USL_Ptn.png
grid_br_usl_p/Brochure_Grid_USL_P.png
  DESTINATION ${TEMPLATEDIR}grid_poster_a0
)

install(FILES
grid_br_usl_l/Brochure_Grid_USL_L.sla.gz
grid_br_usl_l/Brochure_Grid_USL_Ltn.png
grid_br_usl_l/Brochure_Grid_USL_L.png
  DESTINATION ${TEMPLATEDIR}grid_br_usl_l
)

install(FILES
grid_br_dinl_p/Brochure_Grid_DIN_LANG_P.sla.gz
grid_br_dinl_p/Brochure_Grid_DIN_LANG_Ptn.png
grid_br_dinl_p/Brochure_Grid_DIN_LANG_P.png
  DESTINATION ${TEMPLATEDIR}grid_br_dinl_p
)

install(FILES
grid_br_dinl_l/Brochure_Grid_DIN_LANG_L.sla.gz
grid_br_dinl_l/Brochure_Grid_DIN_LANG_Ltn.png
grid_br_dinl_l/Brochure_Grid_DIN_LANG_L.png
  DESTINATION ${TEMPLATEDIR}grid_br_dinl_l
)

install(FILES
grid_br_a5_p/Brochure_Grid_A5_P.sla.gz
grid_br_a5_p/Brochure_Grid_A5_Portraittn.png
grid_br_a5_p/Brochure_Grid_A5_Portrait.png
  DESTINATION ${TEMPLATEDIR}grid_br_a5_p
)

install(FILES
grid_br_a5_l/Brochure_Grid_A5_L.sla.gz
grid_br_a5_l/Brochure_Grid_A5_Ltn.png
grid_br_a5_l/Brochure_Grid_A5_L.png
  DESTINATION ${TEMPLATEDIR}grid_br_a5_l
)

install(FILES
buscard_coll/business_card_collection.sla.gz
buscard_coll/Business_Card_Collectiontn.png
buscard_coll/Business_Card_Collection.png
  DESTINATION ${TEMPLATEDIR}buscard_coll
)

INSTALL(FILES
Lulu_and_Createspace_6x9_paper_book/Lulu_and_Createspace_6x9_paper_book.png
Lulu_and_Createspace_6x9_paper_book/Lulu_and_Createspace_6x9_paper_book.sla.gz
Lulu_and_Createspace_6x9_paper_book/Lulu_and_Createspace_6x9_paper_booktn.png
Lulu_and_Createspace_6x9_paper_book/Tomato_Clipart.png
  DESTINATION ${TEMPLATEDIR}Lulu_and_Createspace_6x9_paper_book
)

install(FILES
ad_sixth_bw/ad_sixth_bw.png
ad_sixth_bw/ad_sixth_bwtn.png
ad_sixth_bw/ad_sixth_bw.sla.gz
  DESTINATION ${TEMPLATEDIR}ad_sixth_bw
)

install(FILES
ad_a6_bw/ad_a6_bw.png
ad_a6_bw/ad_a6_bwtn.png
ad_a6_bw/ad_a6_bw.sla.gz
  DESTINATION ${TEMPLATEDIR}ad_a6_bw
)

install(FILES
ad_chairs_bw/ad_chairs_bw.png
ad_chairs_bw/ad_chairs_bwtn.png
ad_chairs_bw/ad_chairs_bw.sla.gz
  DESTINATION ${TEMPLATEDIR}ad_chairs_bw
)

install(FILES
ad_company_bw/ad_company_bw.png
ad_company_bw/ad_company_bwtn.png
ad_company_bw/ad_company_bw.sla.gz
  DESTINATION ${TEMPLATEDIR}ad_company_bw
)

install(FILES
poster_garage_sale/poster_garage_sale.png
poster_garage_sale/poster_garage_saletn.png
poster_garage_sale/poster_garage_sale.sla.gz
  DESTINATION ${TEMPLATEDIR}poster_garage_sale
)

install(FILES
flyer_pizza_menu/flyer_pizza_menu.png
flyer_pizza_menu/flyer_pizza_menutn.png
flyer_pizza_menu/flyer_pizza_menu.sla.gz
flyer_pizza_menu/pizza.jpg
  DESTINATION ${TEMPLATEDIR}flyer_pizza_menu
)

install(FILES
menu_takeout_color/menu_takeout_color.png
menu_takeout_color/menu_takeout_colortn.png
menu_takeout_color/menu_takeout_color.sla.gz
menu_takeout_color/teriyaki.png
  DESTINATION ${TEMPLATEDIR}menu_takeout_color
)

install(FILES
menu_takeout_bw/menu_takeout_bw.png
menu_takeout_bw/menu_takeout_bwtn.png
menu_takeout_bw/menu_takeout_bw.sla.gz
menu_takeout_bw/teriyaki-bw.png
  DESTINATION ${TEMPLATEDIR}menu_takeout_bw
)

install(FILES
brochure_nightlife/brochure_nightlife.png
brochure_nightlife/brochure_nightlifetn.png
brochure_nightlife/brochure_nightlife.sla.gz
brochure_nightlife/brightness.jpg
brochure_nightlife/halfmoon.jpg
brochure_nightlife/moon.jpg
brochure_nightlife/nature.jpg
brochure_nightlife/sunset.jpg
  DESTINATION ${TEMPLATEDIR}brochure_nightlife
)

install(FILES
newsletter_bw_a4/newsletter_bw_a4.png
newsletter_bw_a4/newsletter_bw_a4tn.png
newsletter_bw_a4/newsletter_bw_a4.sla.gz
newsletter_bw_a4/kites-bw.png
  DESTINATION ${TEMPLATEDIR}newsletter_bw_a4
)

install(FILES
program_theatre/program_theatre.png
program_theatre/program_theatretn.png
program_theatre/program_theatre.sla.gz
  DESTINATION ${TEMPLATEDIR}program_theatre
)

install(FILES
calendar_a4_landscape/calendar_a4_landscape.png
calendar_a4_landscape/calendar_a4_landscapetn.png
calendar_a4_landscape/calendar_a4_landscape.sla.gz
  DESTINATION ${TEMPLATEDIR}calendar_a4_landscape
)

install(FILES
calendar_a4_photo/calendar_a4_photo.png
calendar_a4_photo/calendar_a4_phototn.png
calendar_a4_photo/calendar_a4_photo.sla.gz
calendar_a4_photo/moon.jpg
  DESTINATION ${TEMPLATEDIR}calendar_a4_photo
)

install(FILES
news_school_admission_flyer/news_school_admission_flyer.png
news_school_admission_flyer/news_school_admission_flyertn.png
news_school_admission_flyer/news_school_admission_flyer.sla.gz
  DESTINATION ${TEMPLATEDIR}news_school_admission_flyer
)

install(FILES
news_disp_hospital_4x15/news_disp_hospital_4x15.png
news_disp_hospital_4x15/news_disp_hospital_4x15tn.png
news_disp_hospital_4x15/news_disp_hospital_4x15.sla.gz
  DESTINATION ${TEMPLATEDIR}news_disp_hospital_4x15
)

install(FILES
news_disp_sale_full_page/news_disp_sale_full_page.png
news_disp_sale_full_page/news_disp_sale_full_pagetn.png
news_disp_sale_full_page/news_disp_sale_full_page.sla.gz
  DESTINATION ${TEMPLATEDIR}news_disp_sale_full_page
)

install(FILES
news_disp_jewellery_half_horizontal/news_disp_jewellery_half_horizontal.png
news_disp_jewellery_half_horizontal/news_disp_jewellery_half_horizontaltn.png
news_disp_jewellery_half_horizontal/news_disp_jewellery_half_horizontal.sla.gz
  DESTINATION ${TEMPLATEDIR}news_disp_jewellery_half_horizontal
)

install(FILES
news_disp_plots_half_vertical/news_disp_plots_half_vertical.png
news_disp_plots_half_vertical/news_disp_plots_half_verticaltn.png
news_disp_plots_half_vertical/news_disp_plots_half_vertical.sla.gz
  DESTINATION ${TEMPLATEDIR}news_disp_plots_half_vertical
)

install(FILES
news_disp_school_5x20/news_disp_school_5x20.png
news_disp_school_5x20/news_disp_school_5x20tn.png
news_disp_school_5x20/news_disp_school_5x20.sla.gz
  DESTINATION ${TEMPLATEDIR}news_disp_school_5x20
)

install(FILES
news_disp_car_quarter_page/news_disp_car_quarter_page.png
news_disp_car_quarter_page/news_disp_car_quarter_pagetn.png
news_disp_car_quarter_page/news_disp_car_quarter_page.sla.gz
  DESTINATION ${TEMPLATEDIR}news_disp_car_quarter_page
)

install(FILES
news_disp_electronics_4x10/news_disp_electronics_4x10.png
news_disp_electronics_4x10/news_disp_electronics_4x10tn.png
news_disp_electronics_4x10/news_disp_electronics_4x10.sla.gz
  DESTINATION ${TEMPLATEDIR}news_disp_electronics_4x10
)

install(FILES
news_disp_furniture_4x12/news_disp_furniture_4x12.png
news_disp_furniture_4x12/news_disp_furniture_4x12tn.png
news_disp_furniture_4x12/news_disp_furniture_4x12.sla.gz
  DESTINATION ${TEMPLATEDIR}news_disp_furniture_4x12
)

install(FILES
news_disp_diwali_5x15/news_disp_diwali_5x15.png
news_disp_diwali_5x15/news_disp_diwali_5x15tn.png
news_disp_diwali_5x15/news_disp_diwali_5x15.sla.gz
  DESTINATION ${TEMPLATEDIR}news_disp_diwali_5x15
)

install(FILES
news_disp_holi_4x10/news_disp_holi_4x10.png
news_disp_holi_4x10/news_disp_holi_4x10tn.png
news_disp_holi_4x10/news_disp_holi_4x10.sla.gz
  DESTINATION ${TEMPLATEDIR}news_disp_holi_4x10
)

install(FILES
news_disp_eid_3x10/news_disp_eid_3x10.png
news_disp_eid_3x10/news_disp_eid_3x10tn.png
news_disp_eid_3x10/news_disp_eid_3x10.sla.gz
  DESTINATION ${TEMPLATEDIR}news_disp_eid_3x10
)

install(FILES
news_disp_independence_5x10/news_disp_independence_5x10.png
news_disp_independence_5x10/news_disp_independence_5x10tn.png
news_disp_independence_5x10/news_disp_independence_5x10.sla.gz
  DESTINATION ${TEMPLATEDIR}news_disp_independence_5x10
)

install(FILES
news_disp_sweets_4x10/news_disp_sweets_4x10.png
news_disp_sweets_4x10/news_disp_sweets_4x10tn.png
news_disp_sweets_4x10/news_disp_sweets_4x10.sla.gz
  DESTINATION ${TEMPLATEDIR}news_disp_sweets_4x10
)

install(FILES
news_disp_restaurant_5x12/news_disp_restaurant_5x12.png
news_disp_restaurant_5x12/news_disp_restaurant_5x12tn.png
news_disp_restaurant_5x12/news_disp_restaurant_5x12.sla.gz
  DESTINATION ${TEMPLATEDIR}news_disp_restaurant_5x12
)

install(FILES
news_disp_grand_opening_4x15/news_disp_grand_opening_4x15.png
news_disp_grand_opening_4x15/news_disp_grand_opening_4x15tn.png
news_disp_grand_opening_4x15/news_disp_grand_opening_4x15.sla.gz
  DESTINATION ${TEMPLATEDIR}news_disp_grand_opening_4x15
)

install(FILES
news_disp_twowheeler_4x10/news_disp_twowheeler_4x10.png
news_disp_twowheeler_4x10/news_disp_twowheeler_4x10tn.png
news_disp_twowheeler_4x10/news_disp_twowheeler_4x10.sla.gz
  DESTINATION ${TEMPLATEDIR}news_disp_twowheeler_4x10
)

install(FILES
news_disp_coaching_3x10/news_disp_coaching_3x10.png
news_disp_coaching_3x10/news_disp_coaching_3x10tn.png
news_disp_coaching_3x10/news_disp_coaching_3x10.sla.gz
  DESTINATION ${TEMPLATEDIR}news_disp_coaching_3x10
)

install(FILES
news_disp_matrimonial_bw_3x10/news_disp_matrimonial_bw_3x10.png
news_disp_matrimonial_bw_3x10/news_disp_matrimonial_bw_3x10tn.png
news_disp_matrimonial_bw_3x10/news_disp_matrimonial_bw_3x10.sla.gz
  DESTINATION ${TEMPLATEDIR}news_disp_matrimonial_bw_3x10
)

install(FILES
news_disp_vacancy_bw_3x10/news_disp_vacancy_bw_3x10.png
news_disp_vacancy_bw_3x10/news_disp_vacancy_bw_3x10tn.png
news_disp_vacancy_bw_3x10/news_disp_vacancy_bw_3x10.sla.gz
  DESTINATION ${TEMPLATEDIR}news_disp_vacancy_bw_3x10
)

install(FILES
news_disp_tender_bw_4x10/news_disp_tender_bw_4x10.png
news_disp_tender_bw_4x10/news_disp_tender_bw_4x10tn.png
news_disp_tender_bw_4x10/news_disp_tender_bw_4x10.sla.gz
  DESTINATION ${TEMPLATEDIR}news_disp_tender_bw_4x10
)

install(FILES
news_disp_auction_notice_bw_5x15/news_disp_auction_notice_bw_5x15.png
news_disp_auction_notice_bw_5x15/news_disp_auction_notice_bw_5x15tn.png
news_disp_auction_notice_bw_5x15/news_disp_auction_notice_bw_5x15.sla.gz
  DESTINATION ${TEMPLATEDIR}news_disp_auction_notice_bw_5x15
)

install(FILES
news_disp_strip_8x8_loan_mela/news_disp_strip_8x8_loan_mela.png
news_disp_strip_8x8_loan_mela/news_disp_strip_8x8_loan_melatn.png
news_disp_strip_8x8_loan_mela/news_disp_strip_8x8_loan_mela.sla.gz
  DESTINATION ${TEMPLATEDIR}news_disp_strip_8x8_loan_mela
)

install(FILES
news_disp_clinic_3x10/news_disp_clinic_3x10.png
news_disp_clinic_3x10/news_disp_clinic_3x10tn.png
news_disp_clinic_3x10/news_disp_clinic_3x10.sla.gz
  DESTINATION ${TEMPLATEDIR}news_disp_clinic_3x10
)

install(FILES
news_disp_jacket_deepotsav_full_page/news_disp_jacket_deepotsav_full_page.png
news_disp_jacket_deepotsav_full_page/news_disp_jacket_deepotsav_full_pagetn.png
news_disp_jacket_deepotsav_full_page/news_disp_jacket_deepotsav_full_page.sla.gz
  DESTINATION ${TEMPLATEDIR}news_disp_jacket_deepotsav_full_page
)

install(FILES
news_disp_namechange_bw_2x8/news_disp_namechange_bw_2x8.png
news_disp_namechange_bw_2x8/news_disp_namechange_bw_2x8tn.png
news_disp_namechange_bw_2x8/news_disp_namechange_bw_2x8.sla.gz
  DESTINATION ${TEMPLATEDIR}news_disp_namechange_bw_2x8
)

endif (WANT_NOTEMPLATES)
