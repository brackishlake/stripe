<?php

/**
 * @file
 * Stripe administration and module settings UI.
 */

/**
 * Menu callback: configure Stripe API Keys
 */
function stripe_admin_keys() {
	$form['#tree'] = TRUE;

  $form['API_Keys'] = array(
    '#type' => 'fieldset',
    '#title' => t('API Keys'),
    '#collapsible' => FALSE,
  );

  $fields = array(
    'publishable_test',
    'publishable_live',
    'secret_test',
    'secret_live',
  );

  $active_key = variable_get('stripe_active_key', '');
  foreach ($fields as $field) {
    $title = ucwords(str_replace('_', ' ', $field));
    $form['API_Keys']['stripe_' . $field] = array(
      '#type' => 'textfield',
      '#title' => t($title),
      '#size' => 35,
      '#default_value' => variable_get('stripe_' . $field, ''),
    );
    $form['API_Keys']['active_' . $field] = array(
      '#prefix' => '<div class="container-inline form-item">',
      '#suffix' => '</div>'
    );
    $form['API_Keys']['active_' . $field][$field] = array(
      '#type' => 'radio',
      '#title' => ($active_key == $field) ? t('Active') : t('Activate'),
      '#return_value' => $field,
      '#default_value' => ($active_key == $field) ? $active_key : 0,
      '#parents' => array('stripe_active_key'),
    );
    if ($active_key == $field) {
      $form['API_Keys']['active_' . $field][$field]['#attributes'] = array('class' => array('active'));
      $form['API_Keys']['active_' . $field][$field]['#title'] = t('Active');
    }
  }

  // Attach a little custom css.
  $form['#attached']['css'] = array(
    drupal_get_path('module', 'stripe') . '/stripe.css',
  );

  // Attach a little custom js.
  $form['#attached']['js'] = array(
    drupal_get_path('module', 'stripe') . '/stripe.js',
  );

  $form['#submit'][] = 'stripe_admin_config_submit';

  return system_settings_form($form);
}

/**
 * Process result from stripe config form.
 */
function stripe_admin_config_submit($form, &$form_state) {
	foreach($form_state['values']['API_Keys'] as $field => $value) {
		variable_set($field, $value);
	}
	
	variable_set('stripe_active_key', $form_state['values']['stripe_active_key']);
}

function stripe_admin_test(&$form_state) {
  $form['card_number'] = array(
    '#type' => 'textfield',
    '#title' => t('Card Number:'),
    '#size' => 20,
    '#default_value' => '4242424242424242',
    '#attributes' => array(
      'autocomplete' => 'off',
      'class' => 'card-number',
    ),
  );

  $form['card_cvc'] = array(
    '#type' => 'textfield',
    '#title' => t('CVC:'),
    '#size' => 4,
    '#default_value' => '123',
    '#attributes' => array(
      'autocomplete' => 'off',
      'class' => 'card-cvc',
    ),
  );

  $form['card_expiry_month'] = array(
    '#type' => 'textfield',
    '#title' => t('Expiration Month'),
    '#size' => 2,
    '#default_value' => '10',
    '#attributes' => array(
      'class' => 'card-expiry-month',
    ),
  );

  $form['card_expiry_year'] = array(
    '#type' => 'textfield',
    '#title' => t('Expiration Year'),
    '#size' => 2,
    '#default_value' => '2020',
    '#attributes' => array(
      'class' => 'card-expiry-year',
    ),
  );

  $form['amount'] = array(
    '#type' => 'textfield',
    '#title' => t('Amount:'),
    '#prefix' => '<div class="container-inline form-item">',
    '#suffix' => '<em>cents</em></div>',
    '#default_value' => 99,
    '#size' => 6,
    '#attributes' => array(
      'class' => 'amount',
    ),
  );

  // Retrieve the active API Key.
  $active_key_name = variable_get('stripe_active_key', '');
  $active_key = variable_get('stripe_' . $active_key_name, '');

  $form_state['storage']['key'] = $active_key;
  
  if ($active_key !== '') {
  	drupal_add_js("Stripe.setPublishableKey('{$active_key}');", 'inline');
  	
  	drupal_add_js(drupal_get_path('module', 'stripe') . '/stripe.js', 'file');
  	
  	$settings['stripeConfig'] = array();
	
		drupal_add_js($settings, 'setting');
  }
  
  $form['card_submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit Payment'),
    '#attributes' => array(
    'class' => array('submit-button'),
    ),
    '#disabled' => ($active_key !== '') ? false : true,
  );

  return $form;
}

function stripe_admin_menu() {

}

/**
 * Submit handler for the test form.
 */
function stripe_admin_test_submit($form, &$form_state) {
	$token = $_POST['stripeToken'];
	
	Stripe::setApiKey($form_state['storage']['key']);
	
	$charge = Stripe_Charge::create(array(
		"amount" => $form_state['values']['amount'],
		"currency" => "usd",
		"card" => $token,
		"description" => "payinguser@example.com")
	);
}

/**
 * A list of payments received. There's no local storage for this information, so it's a request every time right now.
 * This also needs pagination. Default response for Stripe is 10, offset 0.
 */
function stripe_admin_payments() {
	$active_key_name = variable_get('stripe_active_key', '');
	$active_key = variable_get('stripe_' . $active_key_name, '');

	$header = array(
		t('Date'),
		t('Name'),
		t('Status'),
		t('Total'),
    );
    
    $rows = array();
    
    Stripe::setApiKey($active_key);
    
    $charges = Stripe_Charge::all();
    
    
    foreach($charges['data'] as $charge) {
    	$row = array();
    	
    	$row[] = date('m-d-Y h:i:s A', $charge['created']);
    	$row[] = $charge['card']['name'];
    	$row[] = ($charge['paid'] == 1) ? 'Paid' : 'Unpaid';
    	$row[] = money_format('$%i', stripe_transform_amount($charge['amount']));
    	
    	$rows[] = $row;
    }
	
	return theme('table', $header, $rows);
}

/**
 * Convert from cents to dollars.
 */
function stripe_transform_amount($amount) {
	if( (int)$amount == $amount) {
		$amount = (float)$amount / 100;
	} else {
		$amount = $amount * 100;
	}
	
	return $amount;
}